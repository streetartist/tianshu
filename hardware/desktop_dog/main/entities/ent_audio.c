/**
 * @file ent_audio.c
 * @brief Audio/music playback entity implementation
 *
 * Wraps audio_player component for music playback.
 * Uses uFlow coroutine to monitor playback status.
 */

#include "ent_audio.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"

#include "audio_player.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_AUDIO";

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_audio_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_playing_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_play_request(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_stop_request(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t playback_monitor_flow(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_AUDIO_PLAY, STATE_AUDIO_PLAYING, on_play_request),
    UR_RULE_END
};

/* State: PLAYING */
static const ur_rule_t s_playing_rules[] = {
    UR_RULE(SIG_AUDIO_STOP, STATE_AUDIO_IDLE, on_stop_request),
    UR_RULE(SIG_AUDIO_PLAY, 0, on_play_request),  /* Replace current */
    UR_RULE(SIG_SYS_TIMEOUT, 0, playback_monitor_flow),  /* Tickless */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_AUDIO_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
    UR_STATE(STATE_AUDIO_PLAYING, 0, on_playing_entry, NULL, s_playing_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGD(TAG, "Audio idle");
    UR_FLOW_RESET(ent);
    return 0;
}

static uint16_t on_playing_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Audio playing");
    return 0;
}

/**
 * @brief Handle play request
 *
 * Starts audio playback. Monitoring flow auto-starts via ur_run().
 */
static uint16_t on_play_request(ur_entity_t *ent, const ur_signal_t *sig)
{
    char *url = (char *)sig->ptr;

    if (!url || strlen(url) == 0) {
        ESP_LOGW(TAG, "Invalid URL");
        return 0;
    }

    ESP_LOGI(TAG, "Playing: %.50s...", url);

    /* Stop any current playback */
    if (audio_player_is_playing()) {
        audio_player_stop();
    }

    /* Start playback */
    esp_err_t err = audio_player_play_url(url);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Play failed: %d", err);
        return STATE_AUDIO_IDLE;
    }

    /* Flow auto-starts via ur_run() */
    return 0;
}

/**
 * @brief Handle stop request
 */
static uint16_t on_stop_request(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Stopping audio");
    audio_player_stop();
    return 0;
}

/**
 * @brief Playback monitoring coroutine (tickless)
 *
 * Monitors playback status and emits SIG_AUDIO_DONE when complete.
 * Time-based waiting is auto-driven by dispatch loop.
 */
static uint16_t playback_monitor_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    UR_FLOW_BEGIN(ent);

    while (audio_player_is_playing()) {
        /* Wait before checking again (auto-driven by dispatch) */
        UR_AWAIT_TIME(ent, 500);
    }

    /* Playback completed */
    ESP_LOGI(TAG, "Playback complete");

    /* Notify system */
    ur_emit_to_id(ENT_ID_SYSTEM,
        (ur_signal_t){ .id = SIG_AUDIO_DONE, .src_id = ENT_ID_AUDIO });

    UR_FLOW_GOTO(ent, STATE_AUDIO_IDLE);

    UR_FLOW_END(ent);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_audio_get(void)
{
    return &s_audio_entity;
}

ur_err_t ent_audio_init(void)
{
    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_AUDIO,
        .name = "audio",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_AUDIO_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_audio_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init audio entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Audio entity initialized");
    return UR_OK;
}
