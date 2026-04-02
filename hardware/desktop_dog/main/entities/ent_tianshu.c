/**
 * @file ent_tianshu.c
 * @brief TianShu IoT platform entity implementation
 *
 * Handles heartbeat, data reporting, and command reception.
 * Uses uFlow coroutine for periodic heartbeat.
 */

#include "ent_tianshu.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"
#include "app_payloads.h"

#include "tianshu.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_TS";

/* Static command payload for forwarding to system */
static ts_command_t s_command;
static char s_cmd_name[64];
static char s_cmd_params[256];

/* ============================================================================
 * TianShu Command Callback
 * ========================================================================== */

/**
 * @brief Callback from TianShu SDK when command received
 */
static void tianshu_cmd_callback(const char *cmd, const char *params)
{
    ESP_LOGI(TAG, "TianShu command: %s, params: %s", cmd, params ? params : "");

    /* Handle pet commands directly */
    if (strcmp(cmd, "pet_feed") == 0) {
        ur_emit_to_id(ENT_ID_PET,
            (ur_signal_t){ .id = SIG_PET_INTERACTION, .src_id = ENT_ID_TIANSHU,
                           .payload = { .u8 = { PET_INTERACT_FEED } } });
        return;
    }
    if (strcmp(cmd, "pet_play") == 0) {
        ur_emit_to_id(ENT_ID_PET,
            (ur_signal_t){ .id = SIG_PET_INTERACTION, .src_id = ENT_ID_TIANSHU,
                           .payload = { .u8 = { PET_INTERACT_PLAY } } });
        return;
    }
    if (strcmp(cmd, "pet_sleep") == 0) {
        ur_emit_to_id(ENT_ID_PET,
            (ur_signal_t){ .id = SIG_PET_INTERACTION, .src_id = ENT_ID_TIANSHU,
                           .payload = { .u8 = { PET_INTERACT_SLEEP } } });
        return;
    }
    if (strcmp(cmd, "pet_revive") == 0) {
        ur_emit_to_id(ENT_ID_PET,
            (ur_signal_t){ .id = SIG_PET_INTERACTION, .src_id = ENT_ID_TIANSHU,
                           .payload = { .u8 = { PET_INTERACT_REVIVE } } });
        return;
    }
    if (strcmp(cmd, "pet_set_mode") == 0) {
        /* Set speed mode: params should be "0", "1", or "2" */
        pet_data_t *data = app_get_pet_data();
        if (params && params[0] >= '0' && params[0] <= '2') {
            data->speed_mode = params[0] - '0';
        } else {
            data->speed_mode = 0;
        }
        ESP_LOGI(TAG, "Pet speed mode set to: %d", data->speed_mode);
        return;
    }

    /* Copy to static buffers for other commands */
    strncpy(s_cmd_name, cmd, sizeof(s_cmd_name) - 1);
    s_cmd_name[sizeof(s_cmd_name) - 1] = '\0';

    if (params) {
        strncpy(s_cmd_params, params, sizeof(s_cmd_params) - 1);
        s_cmd_params[sizeof(s_cmd_params) - 1] = '\0';
    } else {
        s_cmd_params[0] = '\0';
    }

    s_command.command = s_cmd_name;
    s_command.params = s_cmd_params;

    /* Forward to system entity */
    ur_emit_to_id(ENT_ID_SYSTEM,
        (ur_signal_t){ .id = SIG_TS_COMMAND, .src_id = ENT_ID_TIANSHU,
                       .ptr = &s_command });
}

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_tianshu_entity;

/* Forward declarations */
static uint16_t on_disconnected_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_connected_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_sensor_data(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_pet_attr_changed(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t heartbeat_flow(ur_entity_t *ent, const ur_signal_t *sig);

/* State: DISCONNECTED */
static const ur_rule_t s_disconnected_rules[] = {
    UR_RULE(SIG_SYSTEM_READY, STATE_TS_CONNECTED, NULL),
    UR_RULE_END
};

/* State: CONNECTED */
static const ur_rule_t s_connected_rules[] = {
    UR_RULE(SIG_SENSOR_DATA, 0, on_sensor_data),
    UR_RULE(SIG_PET_ATTR_CHANGED, 0, on_pet_attr_changed),
    UR_RULE(SIG_TS_HEARTBEAT, 0, heartbeat_flow),
    UR_RULE(SIG_SYS_TIMEOUT, 0, heartbeat_flow),  /* Auto-driven by dispatch */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_TS_DISCONNECTED, 0, on_disconnected_entry, NULL, s_disconnected_rules),
    UR_STATE(STATE_TS_CONNECTED, 0, on_connected_entry, NULL, s_connected_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_disconnected_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TianShu disconnected");
    return 0;
}

static uint16_t on_connected_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TianShu connected, starting heartbeat");

    /* Connect to TianShu */
    tianshu_connect();
    tianshu_ws_connect();

    /* Flow auto-starts via ur_run() */
    return 0;
}

/**
 * @brief Handle sensor data and report to TianShu
 */
static uint16_t on_sensor_data(ur_entity_t *ent, const ur_signal_t *sig)
{
    sensor_data_t *data = (sensor_data_t *)sig->ptr;

    if (!data || !data->valid) {
        return 0;
    }

    /* Report to TianShu */
    ts_datapoint_t points[2] = {
        { .name = "temperature", .type = TS_TYPE_FLOAT, .value.f = data->temperature },
        { .name = "humidity", .type = TS_TYPE_FLOAT, .value.f = data->humidity }
    };

    esp_err_t err = tianshu_report_batch(points, 2);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Data reported: temp=%.1f, hum=%.1f",
                 data->temperature, data->humidity);
    } else {
        ESP_LOGW(TAG, "Data report failed: %d", err);
    }

    return 0;
}

/**
 * @brief Handle pet attribute changes and report to TianShu
 */
static uint16_t on_pet_attr_changed(ur_entity_t *ent, const ur_signal_t *sig)
{
    pet_data_t *data = (pet_data_t *)sig->ptr;

    if (!data) {
        return 0;
    }

    /* Report pet data to TianShu */
    ts_datapoint_t points[4] = {
        { .name = "pet_happiness", .type = TS_TYPE_INT, .value.i = data->stats.happiness },
        { .name = "pet_hunger", .type = TS_TYPE_INT, .value.i = data->stats.hunger },
        { .name = "pet_fatigue", .type = TS_TYPE_INT, .value.i = data->stats.fatigue },
        { .name = "pet_mood", .type = TS_TYPE_INT, .value.i = data->current_mood }
    };

    esp_err_t err = tianshu_report_batch(points, 4);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Pet data reported: H=%d, Hu=%d, F=%d, Mood=%d",
                 data->stats.happiness, data->stats.hunger,
                 data->stats.fatigue, data->current_mood);
    } else {
        ESP_LOGW(TAG, "Pet data report failed: %d", err);
    }

    return 0;
}

/**
 * @brief Heartbeat coroutine (tickless)
 *
 * Periodically sends heartbeat to TianShu server.
 * Time-based waiting is auto-driven by dispatch loop.
 */
static uint16_t heartbeat_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    UR_FLOW_BEGIN(ent);

    while (1) {
        /* Send heartbeat */
        esp_err_t err = tianshu_heartbeat();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Heartbeat sent");
        } else {
            ESP_LOGW(TAG, "Heartbeat failed, reconnecting...");
            tianshu_connect();
        }

        /* Wait for next heartbeat (auto-driven by dispatch) */
        UR_AWAIT_TIME(ent, TIANSHU_HEARTBEAT_SEC * 1000);
    }

    UR_FLOW_END(ent);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_tianshu_get(void)
{
    return &s_tianshu_entity;
}

ur_err_t ent_tianshu_init(void)
{
    /* Initialize TianShu SDK */
    ts_config_t ts_cfg = {
        .server_url = TIANSHU_SERVER,
        .device_id = TIANSHU_DEVICE_ID,
        .secret_key = TIANSHU_SECRET_KEY,
        .heartbeat_interval = TIANSHU_HEARTBEAT_SEC,
        .data_interval = TIANSHU_REPORT_SEC,
        .command_callback = tianshu_cmd_callback
    };

    esp_err_t err = tianshu_init(&ts_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "TianShu init failed: %d (continuing without)", err);
        /* Continue anyway, entity will stay disconnected */
    }

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_TIANSHU,
        .name = "tianshu",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_TS_DISCONNECTED,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_tianshu_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init TianShu entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "TianShu entity initialized");
    return UR_OK;
}
