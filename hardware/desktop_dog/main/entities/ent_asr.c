/**
 * @file ent_asr.c
 * @brief Speech recognition entity implementation
 *
 * Wraps Baidu ASR for speech-to-text conversion.
 * Receives SIG_ASR_REQUEST with audio, emits SIG_ASR_RESULT with text.
 */

#include "ent_asr.h"
#include "ur_core.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"

#include "baidu_asr.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_ASR";

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_asr_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_asr_request(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_ASR_REQUEST, STATE_ASR_PROCESSING, on_asr_request),
    UR_RULE_END
};

/* State: PROCESSING */
static const ur_rule_t s_processing_rules[] = {
    /* Processing is synchronous, transitions back to IDLE immediately */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_ASR_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
    UR_STATE(STATE_ASR_PROCESSING, 0, NULL, NULL, s_processing_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGD(TAG, "ASR idle");
    return 0;
}

/**
 * @brief Handle ASR request
 *
 * Performs synchronous ASR (blocking call) and emits result.
 */
static uint16_t on_asr_request(ur_entity_t *ent, const ur_signal_t *sig)
{
    audio_data_t *audio = (audio_data_t *)sig->ptr;

    if (!audio || !audio->samples || audio->sample_count == 0) {
        ESP_LOGW(TAG, "Invalid audio data");
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_ASR_ERROR, .src_id = ENT_ID_ASR });
        return STATE_ASR_IDLE;
    }

    ESP_LOGI(TAG, "Processing %d samples", audio->sample_count);

    /* Get result buffer */
    char *result = app_get_asr_result_buffer();
    memset(result, 0, ASR_RESULT_SIZE);

    /* Call Baidu ASR (blocking) */
    esp_err_t err = baidu_asr_recognize(
        audio->samples,
        audio->sample_count,
        result,
        ASR_RESULT_SIZE
    );

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ASR failed: %d", err);
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_ASR_ERROR, .src_id = ENT_ID_ASR });
        return STATE_ASR_IDLE;
    }

    if (strlen(result) == 0) {
        ESP_LOGW(TAG, "ASR returned empty result");
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_ASR_ERROR, .src_id = ENT_ID_ASR });
        return STATE_ASR_IDLE;
    }

    ESP_LOGI(TAG, "ASR result: %s", result);

    /* Send result to system */
    ur_emit_to_id(ENT_ID_SYSTEM,
        (ur_signal_t){ .id = SIG_ASR_RESULT, .src_id = ENT_ID_ASR,
                       .ptr = result });

    return STATE_ASR_IDLE;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_asr_get(void)
{
    return &s_asr_entity;
}

ur_err_t ent_asr_init(void)
{
    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_ASR,
        .name = "asr",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_ASR_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_asr_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init ASR entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "ASR entity initialized");
    return UR_OK;
}
