/**
 * @file ent_ai.c
 * @brief AI conversation entity implementation
 *
 * Wraps Mimo AI for chat with tool calling support.
 * Receives SIG_AI_REQUEST with text, emits SIG_AI_RESPONSE.
 */

#include "ent_ai.h"
#include "ur_core.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"

#include "mimo_ai.h"
#include "dyn_string.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_AI";

/* Dynamic string for AI response */
static dyn_string_t *s_ai_answer = NULL;

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_ai_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_ai_request(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_AI_REQUEST, STATE_AI_PROCESSING, on_ai_request),
    UR_RULE_END
};

/* State: PROCESSING */
static const ur_rule_t s_processing_rules[] = {
    /* Processing is synchronous, transitions back to IDLE immediately */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_AI_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
    UR_STATE(STATE_AI_PROCESSING, 0, NULL, NULL, s_processing_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGD(TAG, "AI idle");
    return 0;
}

/**
 * @brief Handle AI request
 *
 * Performs synchronous AI chat (blocking call) and emits response.
 */
static uint16_t on_ai_request(ur_entity_t *ent, const ur_signal_t *sig)
{
    char *question = (char *)sig->ptr;

    if (!question || strlen(question) == 0) {
        ESP_LOGW(TAG, "Invalid question");
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_AI_ERROR, .src_id = ENT_ID_AI });
        return STATE_AI_IDLE;
    }

    ESP_LOGI(TAG, "Processing question: %s", question);

    /* Clear previous answer */
    dyn_string_clear(s_ai_answer);

    /* Call Mimo AI (blocking) */
    esp_err_t err = mimo_ai_chat(question, s_ai_answer);

    if (err != ESP_OK || dyn_string_len(s_ai_answer) == 0) {
        ESP_LOGW(TAG, "AI chat failed: %d", err);
        ur_emit_to_id(ENT_ID_SYSTEM,
            (ur_signal_t){ .id = SIG_AI_ERROR, .src_id = ENT_ID_AI });
        return STATE_AI_IDLE;
    }

    ESP_LOGI(TAG, "AI response: %s", dyn_string_cstr(s_ai_answer));

    /* Update ai_response structure */
    ai_response_t *response = app_get_ai_response();
    response->text = (char *)dyn_string_cstr(s_ai_answer);
    response->continue_chat = mimo_ai_is_continue_chat_pending();
    response->has_tool_call = false;  /* Tool already executed in mimo_ai_chat */

    /* Check for pending actions */
    /* Note: Actions are executed by mimo_ai_execute_pending_actions()
     * after TTS completes without interruption. */

    /* Send response to system */
    ur_emit_to_id(ENT_ID_SYSTEM,
        (ur_signal_t){ .id = SIG_AI_RESPONSE, .src_id = ENT_ID_AI,
                       .ptr = response });

    return STATE_AI_IDLE;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_ai_get(void)
{
    return &s_ai_entity;
}

ur_err_t ent_ai_init(void)
{
    /* Create dynamic string for answers */
    s_ai_answer = dyn_string_create(AI_ANSWER_SIZE);
    if (!s_ai_answer) {
        ESP_LOGE(TAG, "Failed to create answer buffer");
        return UR_ERR_NO_MEMORY;
    }

    /* Initialize Mimo AI module */
    mimo_ai_init();

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_AI,
        .name = "ai",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_AI_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_ai_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init AI entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "AI entity initialized");
    return UR_OK;
}
