/**
 * @file ent_tts.c
 * @brief Text-to-speech entity implementation (event-driven)
 *
 * Wraps Baidu TTS with interrupt support via uFlow coroutine.
 * Uses UR_AWAIT_SIGNAL for pure event-driven operation.
 */

#include "ent_tts.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"

#include "baidu_tts.h"
#include "mimo_ai.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_TTS";

/* ============================================================================
 * Scratchpad for uFlow
 * ========================================================================== */

typedef struct {
    bool stop_requested;    /**< Stop flag set by SIG_TTS_STOP */
} tts_scratch_t;

UR_SCRATCH_STATIC_ASSERT(tts_scratch_t);

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_tts_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_speaking_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_tts_request(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_tts_stop(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t tts_monitor_flow(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_TTS_REQUEST, STATE_TTS_SPEAKING, on_tts_request),
    UR_RULE_END
};

/* State: SPEAKING - event-driven by SIG_TTS_PLAYBACK_DONE */
static const ur_rule_t s_speaking_rules[] = {
    UR_RULE(SIG_TTS_STOP, 0, on_tts_stop),
    UR_RULE(SIG_TTS_PLAYBACK_DONE, 0, tts_monitor_flow),  /* Event-driven by baidu_tts */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_TTS_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
    UR_STATE(STATE_TTS_SPEAKING, 0, on_speaking_entry, NULL, s_speaking_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGD(TAG, "TTS idle");
    UR_FLOW_RESET(ent);
    return 0;
}

static uint16_t on_speaking_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TTS speaking...");

    /* Initialize scratchpad */
    tts_scratch_t *s = UR_SCRATCH_PTR(ent, tts_scratch_t);
    s->stop_requested = false;

    return 0;
}

/**
 * @brief Handle TTS request
 *
 * Starts TTS playback. baidu_tts will send SIG_TTS_PLAYBACK_DONE when complete.
 */
static uint16_t on_tts_request(ur_entity_t *ent, const ur_signal_t *sig)
{
    char *text = (char *)sig->ptr;

    if (!text || strlen(text) == 0) {
        ESP_LOGW(TAG, "Invalid text");
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_TTS_DONE, .src_id = ENT_ID_TTS });
        return STATE_TTS_IDLE;
    }

    ESP_LOGI(TAG, "TTS request: %.50s...", text);

    /* Start async TTS with notification (event-driven) */
    baidu_tts_speak_async(text, ENT_ID_TTS);

    /* Will receive SIG_TTS_PLAYBACK_DONE when playback completes */
    return 0;
}

/**
 * @brief Handle stop request
 */
static uint16_t on_tts_stop(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TTS stop requested");

    tts_scratch_t *s = UR_SCRATCH_PTR(ent, tts_scratch_t);
    s->stop_requested = true;

    /* Stop TTS playback - will trigger SIG_TTS_PLAYBACK_DONE with was_stopped=1 */
    baidu_tts_stop();

    return 0;
}

/**
 * @brief TTS completion handler (event-driven)
 *
 * Called when baidu_tts sends SIG_TTS_PLAYBACK_DONE.
 * Handles both normal completion and interruption.
 */
static uint16_t tts_monitor_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    tts_scratch_t *s = UR_SCRATCH_PTR(ent, tts_scratch_t);

    /* Check if TTS was stopped/interrupted */
    bool was_stopped = (sig->payload.u32[0] == 1) || s->stop_requested;

    if (was_stopped) {
        ESP_LOGI(TAG, "TTS interrupted");

        /* Clear pending AI actions since we were interrupted */
        mimo_ai_clear_pending_actions();

        /* Notify system */
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_TTS_INTERRUPTED, .src_id = ENT_ID_TTS });
    } else {
        /* TTS completed normally */
        ESP_LOGI(TAG, "TTS completed");

        /* Execute pending AI actions */
        mimo_ai_execute_pending_actions();

        /* Notify system */
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_TTS_DONE, .src_id = ENT_ID_TTS });
    }

    /* Transition to idle */
    return STATE_TTS_IDLE;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_tts_get(void)
{
    return &s_tts_entity;
}

ur_err_t ent_tts_init(void)
{
    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_TTS,
        .name = "tts",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_TTS_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_tts_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init TTS entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "TTS entity initialized (event-driven mode)");
    return UR_OK;
}
