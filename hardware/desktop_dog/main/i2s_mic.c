#include "i2s_mic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdlib.h>

#include "ur_core.h"
#include "ur_pipe.h"
#include "app_signals.h"
#include "app_config.h"

#define I2S_BCLK_IO     4
#define I2S_WS_IO       5
#define I2S_DOUT_IO     6
#define I2S_DIN_IO      7
#define HW_SAMPLE_RATE  44100
#define OUT_SAMPLE_RATE 16000
#define RESAMPLE_RATIO  2.75625f  // 44100/16000
#define I2S_MIC_CHANNELS 2
#define I2S_MIC_RAW_BUF_FRAMES 2816
#define I2S_MIC_RAW_BUF_SAMPLES (I2S_MIC_RAW_BUF_FRAMES * I2S_MIC_CHANNELS)

/* Pipe configuration */
#define MIC_PIPE_SIZE       (VAD_FRAME_SIZE * 4 * sizeof(int16_t))  /* 4 frames buffer */
#define MIC_PIPE_TRIGGER    (VAD_FRAME_SIZE * sizeof(int16_t))      /* Trigger when 1 frame ready */

static const char *TAG = "I2S_MIC";
static SemaphoreHandle_t s_mic_mutex = NULL;
static int16_t *s_raw_buf = NULL;
static size_t s_raw_buf_frames = 0;
static volatile bool s_mic_enabled = false;

/* Streaming mode */
static TaskHandle_t s_stream_task = NULL;
static volatile bool s_streaming = false;
static uint16_t s_recorder_entity_id = 0;
static uint8_t s_pipe_buffer[MIC_PIPE_SIZE];
static ur_pipe_t s_mic_pipe;

i2s_chan_handle_t g_tx_handle = NULL;
i2s_chan_handle_t g_rx_handle = NULL;

/**
 * @brief Background task for streaming mic data to pipe
 */
static void mic_stream_task(void *arg)
{
    int16_t *resample_buf = heap_caps_malloc(VAD_FRAME_SIZE * sizeof(int16_t),
                                              MALLOC_CAP_INTERNAL);
    if (!resample_buf) {
        ESP_LOGE(TAG, "Failed to allocate resample buffer");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Mic stream task started");

    while (s_streaming) {
        /* Read and resample */
        size_t samples = i2s_mic_read(resample_buf, VAD_FRAME_SIZE);

        if (samples > 0) {
            /* Write to pipe */
            size_t written = ur_pipe_write(&s_mic_pipe, resample_buf,
                                           samples * sizeof(int16_t), 0);

            /* Notify recorder when enough data available */
            if (ur_pipe_available(&s_mic_pipe) >= MIC_PIPE_TRIGGER) {
                ur_emit_to_id(s_recorder_entity_id,
                    (ur_signal_t){ .id = SIG_MIC_DATA_READY, .src_id = 0 });
            }

            (void)written;
        }
    }

    free(resample_buf);
    ESP_LOGI(TAG, "Mic stream task stopped");
    vTaskDelete(NULL);
}

esp_err_t i2s_mic_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 1024;
    chan_cfg.auto_clear = true;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &g_tx_handle, &g_rx_handle));

    if (!s_mic_mutex) {
        s_mic_mutex = xSemaphoreCreateMutex();
        if (!s_mic_mutex) return ESP_ERR_NO_MEM;
    }

    if (!s_raw_buf) {
        s_raw_buf = heap_caps_malloc(I2S_MIC_RAW_BUF_SAMPLES * sizeof(int16_t),
                                      MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (!s_raw_buf) return ESP_ERR_NO_MEM;
        s_raw_buf_frames = I2S_MIC_RAW_BUF_FRAMES;
    }

    i2s_std_config_t rx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(HW_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_DIN_IO,
            .invert_flags = { false, false, false },
        },
    };
    rx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT | I2S_STD_SLOT_RIGHT;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(g_rx_handle, &rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(g_rx_handle));
    s_mic_enabled = true;

    /* Initialize pipe */
    ur_pipe_init(&s_mic_pipe, s_pipe_buffer, MIC_PIPE_SIZE, MIC_PIPE_TRIGGER);

    ESP_LOGI(TAG, "I2S mic initialized at %d Hz (output %d Hz)", HW_SAMPLE_RATE, OUT_SAMPLE_RATE);
    return ESP_OK;
}

size_t i2s_mic_read(int16_t *buffer, size_t samples)
{
    if (!s_mic_enabled || !buffer || samples == 0) return 0;

    if (xSemaphoreTake(s_mic_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return 0;

    if (!s_mic_enabled || !s_raw_buf) {
        xSemaphoreGive(s_mic_mutex);
        return 0;
    }

    // 计算需要读取的44.1kHz样本数
    size_t need_hw_frames = (size_t)(samples * RESAMPLE_RATIO) + 1;
    if (need_hw_frames > s_raw_buf_frames) need_hw_frames = s_raw_buf_frames;

    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(g_rx_handle, s_raw_buf,
                                      need_hw_frames * I2S_MIC_CHANNELS * sizeof(int16_t),
                                      &bytes_read, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        xSemaphoreGive(s_mic_mutex);
        return 0;
    }

    size_t hw_frames = bytes_read / (sizeof(int16_t) * I2S_MIC_CHANNELS);
    size_t out_samples = (size_t)(hw_frames / RESAMPLE_RATIO);
    if (out_samples > samples) out_samples = samples;

    // 降采样 44.1kHz -> 16kHz
    for (size_t i = 0; i < out_samples; i++) {
        size_t src_frame = (size_t)(i * RESAMPLE_RATIO);
        if (src_frame < hw_frames) {
            buffer[i] = s_raw_buf[src_frame * I2S_MIC_CHANNELS];
        }
    }

    xSemaphoreGive(s_mic_mutex);
    return out_samples;
}

esp_err_t i2s_mic_disable(void)
{
    ESP_LOGI(TAG, "Disabling mic");
    xSemaphoreTake(s_mic_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (s_mic_enabled) {
        ret = i2s_channel_disable(g_rx_handle);
        if (ret == ESP_OK) s_mic_enabled = false;
    }
    xSemaphoreGive(s_mic_mutex);
    return ret;
}

esp_err_t i2s_mic_enable(void)
{
    ESP_LOGI(TAG, "Enabling mic");
    xSemaphoreTake(s_mic_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (!s_mic_enabled) {
        ret = i2s_channel_enable(g_rx_handle);
        if (ret == ESP_OK) s_mic_enabled = true;
    }
    xSemaphoreGive(s_mic_mutex);
    return ret;
}

esp_err_t i2s_mic_start_stream(uint16_t recorder_entity_id)
{
    if (s_streaming) {
        return ESP_OK;  /* Already streaming */
    }

    s_recorder_entity_id = recorder_entity_id;
    s_streaming = true;

    /* Reset pipe */
    ur_pipe_reset(&s_mic_pipe);

    /* Create stream task */
    BaseType_t ret = xTaskCreatePinnedToCore(
        mic_stream_task,
        "mic_stream",
        4096,
        NULL,
        6,  /* Higher priority than dispatch */
        &s_stream_task,
        0   /* Core 0 */
    );

    if (ret != pdPASS) {
        s_streaming = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Mic streaming started");
    return ESP_OK;
}

esp_err_t i2s_mic_stop_stream(void)
{
    if (!s_streaming) {
        return ESP_OK;
    }

    s_streaming = false;

    /* Wait for task to finish */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Mic streaming stopped");
    return ESP_OK;
}

void *i2s_mic_get_pipe(void)
{
    return &s_mic_pipe;
}
