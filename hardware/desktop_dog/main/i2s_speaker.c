#include "i2s_speaker.h"
#include "i2s_mic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define I2S_BCLK_IO     4
#define I2S_WS_IO       5
#define I2S_DOUT_IO     6
#define I2S_DIN_IO      7
#define SAMPLE_RATE     44100

static const char *TAG = "I2S_SPK";
static SemaphoreHandle_t s_spk_mutex = NULL;
static bool s_spk_enabled = false;
static bool s_spk_initialized = false;
static int16_t *s_play_buf = NULL;
static size_t s_play_buf_size = 0;

esp_err_t i2s_speaker_init(void)
{
    if (!s_spk_mutex) {
        s_spk_mutex = xSemaphoreCreateMutex();
        if (!s_spk_mutex) return ESP_ERR_NO_MEM;
    }

    if (!g_tx_handle) {
        ESP_LOGE(TAG, "TX handle not ready");
        return ESP_ERR_INVALID_STATE;
    }

    i2s_std_config_t tx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
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
    tx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT | I2S_STD_SLOT_RIGHT;

    esp_err_t ret = i2s_channel_init_std_mode(g_tx_handle, &tx_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init TX: %s", esp_err_to_name(ret));
        return ret;
    }

    s_spk_initialized = true;
    ESP_LOGI(TAG, "I2S speaker initialized at %d Hz", SAMPLE_RATE);
    return ESP_OK;
}

esp_err_t i2s_speaker_start(void)
{
    if (!s_spk_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_spk_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (!s_spk_enabled) {
        ret = i2s_channel_enable(g_tx_handle);
        if (ret == ESP_OK) {
            s_spk_enabled = true;
            ESP_LOGI(TAG, "Speaker started");
        }
    }
    xSemaphoreGive(s_spk_mutex);
    return ret;
}

esp_err_t i2s_speaker_play(const uint8_t *data, size_t len)
{
    if (!data || len == 0 || !s_spk_initialized) return ESP_OK;

    xSemaphoreTake(s_spk_mutex, portMAX_DELAY);

    if (!s_spk_enabled) {
        esp_err_t ret = i2s_channel_enable(g_tx_handle);
        if (ret != ESP_OK) {
            xSemaphoreGive(s_spk_mutex);
            return ret;
        }
        s_spk_enabled = true;
    }

    size_t aligned_len = len & ~1;
    if (aligned_len == 0) {
        xSemaphoreGive(s_spk_mutex);
        return ESP_OK;
    }

    size_t samples = aligned_len / 2;
    size_t need_size = samples * 2 * sizeof(int16_t);

    if (need_size > s_play_buf_size) {
        if (s_play_buf) heap_caps_free(s_play_buf);
        s_play_buf = heap_caps_malloc(need_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!s_play_buf) {
            s_play_buf_size = 0;
            xSemaphoreGive(s_spk_mutex);
            return ESP_ERR_NO_MEM;
        }
        s_play_buf_size = need_size;
    }
    // Duplicate mono samples to stereo.
    const int16_t *data16 = (const int16_t *)data;
    for (size_t i = 0; i < samples; i++) {
        int16_t sample = data16[i];
        s_play_buf[i * 2] = sample;
        s_play_buf[i * 2 + 1] = sample;
    }

    size_t bytes_written = 0;
    esp_err_t ret = i2s_channel_write(g_tx_handle, s_play_buf, need_size,
                                       &bytes_written, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
    }

    xSemaphoreGive(s_spk_mutex);
    return ret;
}

esp_err_t i2s_speaker_stop(void)
{
    if (!s_spk_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_spk_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (s_spk_enabled) {
        ret = i2s_channel_disable(g_tx_handle);
        if (ret == ESP_OK) {
            s_spk_enabled = false;
            ESP_LOGI(TAG, "Speaker stopped");
        }
    }
    xSemaphoreGive(s_spk_mutex);
    return ret;
}
