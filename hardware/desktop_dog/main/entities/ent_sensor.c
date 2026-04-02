/**
 * @file ent_sensor.c
 * @brief DHT11 sensor entity implementation
 *
 * Periodically reads temperature and humidity from DHT11.
 * Uses uFlow coroutine for timed readings.
 */

#include "ent_sensor.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"

#include "dht11.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "ENT_SENSOR";

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_sensor_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t sensor_read_flow(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE - uses uFlow for periodic reading (tickless) */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_SENSOR_READ, 0, sensor_read_flow),
    UR_RULE(SIG_SYS_TIMEOUT, 0, sensor_read_flow),  /* Auto-driven by dispatch */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_SENSOR_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Sensor entity started");
    /* Flow auto-starts via ur_run() */
    return 0;
}

/**
 * @brief Sensor reading coroutine (tickless)
 *
 * Periodically reads DHT11 and emits SIG_SENSOR_DATA.
 * Time-based waiting is auto-driven by dispatch loop.
 */
static uint16_t sensor_read_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    UR_FLOW_BEGIN(ent);

    while (1) {
        /* Read DHT11 */
        float temperature, humidity;
        esp_err_t err = dht11_read(&temperature, &humidity);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.1f°C, Humidity: %.1f%%",
                     temperature, humidity);

            /* Update sensor data buffer */
            sensor_data_t *data = app_get_sensor_data();
            data->temperature = temperature;
            data->humidity = humidity;
            data->valid = true;

            /* Emit sensor data signal with temperature in payload */
            ur_signal_t sig_data = {
                .id = SIG_SENSOR_DATA,
                .src_id = ENT_ID_SENSOR,
                .ptr = data
            };
            sig_data.payload.f32 = temperature;

            /* Send to TianShu entity for reporting */
            ur_emit_to_id(ENT_ID_TIANSHU, sig_data);
        } else {
            ESP_LOGW(TAG, "DHT11 read failed");

            sensor_data_t *data = app_get_sensor_data();
            data->valid = false;
        }

        /* Wait for next reading interval (auto-driven by dispatch) */
        UR_AWAIT_TIME(ent, SENSOR_READ_INTERVAL_MS);
    }

    UR_FLOW_END(ent);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_sensor_get(void)
{
    return &s_sensor_entity;
}

ur_err_t ent_sensor_init(void)
{
    /* Initialize DHT11 */
    esp_err_t err = dht11_init(DHT11_GPIO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init DHT11: %d", err);
        return UR_ERR_INVALID_STATE;
    }

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_SENSOR,
        .name = "sensor",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_SENSOR_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_sensor_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init sensor entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Sensor entity initialized");
    return UR_OK;
}
