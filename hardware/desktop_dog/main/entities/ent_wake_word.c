/**
 * @file ent_wake_word.c
 * @brief Wake word detection entity implementation
 *
 * Uses a dedicated FreeRTOS task for continuous audio monitoring
 * because i2s_mic_read is blocking and would stall the dispatch loop.
 * Emits SIG_WAKE_DETECTED when wake word is detected.
 */

#include "ent_wake_word.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"

#include "wake_word.h"
#include "i2s_mic.h"
#include "baidu_tts.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_WAKE";

/* ============================================================================
 * Wake Word Detection Task
 * ========================================================================== */

/* Frame buffer for wake word detection */
static int16_t s_wake_frame_buf[VAD_FRAME_SIZE];

/* Task control */
static TaskHandle_t s_wake_task_handle = NULL;
static volatile bool s_wake_enabled = false;

/**
 * @brief Dedicated task for wake word detection
 *
 * Runs independently of the MicroReactor dispatch loop since
 * i2s_mic_read is blocking.
 */
static void wake_detect_task(void *arg)
{
    ESP_LOGI(TAG, "Wake detection task started");

    while (1) {
        /* Wait until detection is enabled */
        if (!s_wake_enabled) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Read audio frame (blocking) */
        size_t read = i2s_mic_read(s_wake_frame_buf, VAD_FRAME_SIZE);

        if (read > 0 && s_wake_enabled) {
            /* Check for wake word */
            if (wake_word_detect(s_wake_frame_buf, read)) {
                ESP_LOGI(TAG, "Wake word detected!");

                /* Play wake prompt */
                baidu_tts_play_prompt("wake.pcm");

                /* Notify system entity via signal */
                ur_emit_to_id(ENT_ID_SYSTEM,
                    (ur_signal_t){ .id = SIG_WAKE_DETECTED,
                                   .src_id = ENT_ID_WAKE_WORD });
            }
        }

        /* Small delay to prevent tight loop on errors */
        if (read == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_wake_entity;

/* Forward declarations */
static uint16_t on_disabled_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_enabled_entry(ur_entity_t *ent, const ur_signal_t *sig);

/* State: DISABLED */
static const ur_rule_t s_disabled_rules[] = {
    UR_RULE(SIG_WAKE_ENABLE, STATE_WAKE_ENABLED, NULL),
    UR_RULE_END
};

/* State: ENABLED */
static const ur_rule_t s_enabled_rules[] = {
    UR_RULE(SIG_WAKE_DISABLE, STATE_WAKE_DISABLED, NULL),
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_WAKE_DISABLED, 0, on_disabled_entry, NULL, s_disabled_rules),
    UR_STATE(STATE_WAKE_ENABLED, 0, on_enabled_entry, NULL, s_enabled_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_disabled_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Wake word detection disabled");
    s_wake_enabled = false;
    return 0;
}

static uint16_t on_enabled_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Wake word detection enabled");

    /* Create task on first enable (after hardware is initialized) */
    if (s_wake_task_handle == NULL) {
        BaseType_t ret = xTaskCreatePinnedToCore(
            wake_detect_task,
            "wake_detect",
            4096,
            NULL,
            5,
            &s_wake_task_handle,
            0  /* Run on core 0 */
        );
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create wake task");
            return 0;
        }
        ESP_LOGI(TAG, "Wake detection task created");
    }

    s_wake_enabled = true;
    return 0;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_wake_word_get(void)
{
    return &s_wake_entity;
}

ur_err_t ent_wake_word_init(void)
{
    /* Initialize wake word detector */
    esp_err_t err = wake_word_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init wake word: %d", err);
        return UR_ERR_INVALID_STATE;
    }

    /* Task will be created when entity is enabled (after hardware init) */
    s_wake_task_handle = NULL;
    s_wake_enabled = false;

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_WAKE_WORD,
        .name = "wake_word",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_WAKE_DISABLED,
        .user_data = NULL,
    };

    ur_err_t ur_ret = ur_init(&s_wake_entity, &cfg);
    if (ur_ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init wake entity: %d", ur_ret);
        return ur_ret;
    }

    ESP_LOGI(TAG, "Wake word entity initialized");
    return UR_OK;
}
