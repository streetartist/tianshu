/**
 * @file app_buffers.c
 * @brief Desktop Dog static buffer implementations
 *
 * All buffers are statically allocated. Large buffers use PSRAM.
 */

#include "app_buffers.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "APP_BUF";

/* ============================================================================
 * Static Buffers
 * ========================================================================== */

/* Audio buffer in PSRAM (80KB for 5 seconds at 16kHz) */
static int16_t *s_audio_buffer = NULL;

/* Text buffers in internal RAM for faster access */
static char s_asr_result[ASR_RESULT_SIZE];
static char s_ai_answer[AI_ANSWER_SIZE];
static char s_display_text[DISPLAY_TEXT_SIZE];

/* Payload structures */
static audio_data_t s_audio_data;
static ai_response_t s_ai_response;
static sensor_data_t s_sensor_data;
static pet_data_t s_pet_data;

/* ============================================================================
 * Initialization
 * ========================================================================== */

void app_buffers_init(void)
{
    /* Allocate audio buffer in PSRAM */
    s_audio_buffer = heap_caps_malloc(
        AUDIO_BUFFER_SAMPLES * sizeof(int16_t),
        MALLOC_CAP_SPIRAM
    );

    if (!s_audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer in PSRAM");
        /* Fallback to internal RAM if PSRAM not available */
        s_audio_buffer = heap_caps_malloc(
            AUDIO_BUFFER_SAMPLES * sizeof(int16_t),
            MALLOC_CAP_INTERNAL
        );
    }

    if (!s_audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer!");
    } else {
        ESP_LOGI(TAG, "Audio buffer allocated: %d bytes",
                 AUDIO_BUFFER_SAMPLES * sizeof(int16_t));
    }

    /* Clear all buffers */
    memset(s_asr_result, 0, sizeof(s_asr_result));
    memset(s_ai_answer, 0, sizeof(s_ai_answer));
    memset(s_display_text, 0, sizeof(s_display_text));
    memset(&s_audio_data, 0, sizeof(s_audio_data));
    memset(&s_ai_response, 0, sizeof(s_ai_response));
    memset(&s_sensor_data, 0, sizeof(s_sensor_data));
    memset(&s_pet_data, 0, sizeof(s_pet_data));

    /* Initialize pet data with defaults */
    s_pet_data.stats.happiness = 50;
    s_pet_data.stats.hunger = 50;
    s_pet_data.stats.fatigue = 50;
    s_pet_data.current_mood = PET_MOOD_NORMAL;
    s_pet_data.speed_mode = 0;

    /* Setup audio_data structure */
    s_audio_data.samples = s_audio_buffer;
    s_audio_data.sample_rate = AUDIO_SAMPLE_RATE;
    s_audio_data.sample_count = 0;

    /* Setup ai_response structure */
    s_ai_response.text = s_ai_answer;

    ESP_LOGI(TAG, "Buffers initialized");
}

/* ============================================================================
 * Accessors
 * ========================================================================== */

int16_t *app_get_audio_buffer(void)
{
    return s_audio_buffer;
}

char *app_get_asr_result_buffer(void)
{
    return s_asr_result;
}

char *app_get_ai_answer_buffer(void)
{
    return s_ai_answer;
}

audio_data_t *app_get_audio_data(void)
{
    return &s_audio_data;
}

ai_response_t *app_get_ai_response(void)
{
    return &s_ai_response;
}

sensor_data_t *app_get_sensor_data(void)
{
    return &s_sensor_data;
}

char *app_get_display_buffer(void)
{
    return s_display_text;
}

pet_data_t *app_get_pet_data(void)
{
    return &s_pet_data;
}
