/**
 * @file ent_pet.c
 * @brief Pet entity - virtual pet state machine
 *
 * Handles:
 * - Attribute decay on tick
 * - Button interactions
 * - NVS save/load
 * - Display updates
 * - Sound feedback
 */

#include "ent_pet.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"
#include "app_payloads.h"

#include "pet/pet_attributes.h"
#include "pet/pet_storage.h"
#include "pet/pet_display.h"
#include "pet/pet_sound.h"
#include "drivers/button.h"
#include "entities/ent_system.h"
#include "dht11.h"
#include "u8g2.h"

#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ENT_PET";

/* ============================================================================
 * Chart Sampling Timer (1 sample per second for chart history)
 * ========================================================================== */
static esp_timer_handle_t s_chart_timer = NULL;

static void chart_timer_callback(void *arg)
{
    pet_data_t *data = app_get_pet_data();
    pet_display_record_data(data, dht11_get_temperature(), dht11_get_humidity());

    /* If in chart mode and display is active, render chart */
    if (pet_display_is_active() && pet_display_is_chart_mode()) {
        pet_display_render_chart();
    }
}

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_pet_entity;

/* Forward declarations */
static uint16_t on_init_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_active_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_dead_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t tick_flow(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t save_flow(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_button_pressed(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_button_long_pressed(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_pet_interaction(ur_entity_t *ent, const ur_signal_t *sig);

/* State: INIT */
static const ur_rule_t s_init_rules[] = {
    UR_RULE(SIG_SYSTEM_READY, STATE_PET_ACTIVE, NULL),
    UR_RULE_END
};

/* State: ACTIVE */
static const ur_rule_t s_active_rules[] = {
    UR_RULE(SIG_PET_TICK, 0, tick_flow),
    UR_RULE(SIG_SYS_TIMEOUT, 0, tick_flow),
    UR_RULE(SIG_PET_SAVE, 0, save_flow),
    UR_RULE(SIG_BTN_PRESSED, 0, on_button_pressed),
    UR_RULE(SIG_BTN_LONG_PRESSED, 0, on_button_long_pressed),
    UR_RULE(SIG_PET_INTERACTION, 0, on_pet_interaction),
    UR_RULE_END
};

/* State: DEAD */
static const ur_rule_t s_dead_rules[] = {
    UR_RULE(SIG_BTN_LONG_PRESSED, 0, on_button_long_pressed),
    UR_RULE(SIG_PET_INTERACTION, 0, on_pet_interaction),
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_PET_INIT, 0, on_init_entry, NULL, s_init_rules),
    UR_STATE(STATE_PET_ACTIVE, 0, on_active_entry, NULL, s_active_rules),
    UR_STATE(STATE_PET_DEAD, 0, on_dead_entry, NULL, s_dead_rules),
};

/* ============================================================================
 * Button Callback
 * ========================================================================== */

static void button_callback(uint8_t button_id, bool long_press)
{
    ur_signal_t sig = {
        .id = long_press ? SIG_BTN_LONG_PRESSED : SIG_BTN_PRESSED,
        .src_id = ENT_ID_PET,
        .payload = { .u8 = { button_id } }
    };

    ur_emit_to_id(ENT_ID_PET, sig);
}

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_init_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Pet entity initializing...");
    return 0;
}

static uint16_t on_active_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Pet entity active");

    /* Generate pet sound prompts now that WiFi is available */
    pet_sound_init();

    /* Start chart sampling timer (1 sample/sec) */
    if (!s_chart_timer) {
        esp_timer_create_args_t timer_args = {
            .callback = chart_timer_callback,
            .arg = NULL,
            .name = "chart_sample"
        };
        esp_timer_create(&timer_args, &s_chart_timer);
    }
    esp_timer_start_periodic(s_chart_timer, 1000000); /* 1 second */

    pet_data_t *data = app_get_pet_data();

    /* Check if pet is dead on startup */
    if (pet_attributes_is_dead(data)) {
        data->current_mood = PET_MOOD_DEAD;
        return STATE_PET_DEAD;
    }

    /* Update mood and display */
    data->current_mood = pet_attributes_calculate_mood(data);
    pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

    return 0;
}

static uint16_t on_dead_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Pet is dead! Long press BTN4 to revive.");

    pet_data_t *data = app_get_pet_data();
    data->current_mood = PET_MOOD_DEAD;

    /* Play death sound */
    pet_sound_play(PET_SOUND_DEAD);

    /* Update display */
    pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

    /* Broadcast attribute change */
    ur_broadcast((ur_signal_t){
        .id = SIG_PET_ATTR_CHANGED,
        .src_id = ENT_ID_PET,
        .ptr = data
    });

    return 0;
}

/**
 * @brief Tick flow - handles attribute decay
 *
 * Note: Uses Duff's device coroutine. Local variables do NOT persist
 * across UR_AWAIT_TIME yields, so we re-fetch from global each iteration.
 * Cannot return state transitions from inside the loop.
 *
 * Chart data recording is handled by a separate 1Hz timer.
 */
static uint16_t tick_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    UR_FLOW_BEGIN(ent);

    while (1) {
        /* Re-fetch data each iteration (locals don't persist across yields) */
        UR_AWAIT_TIME(ent, app_get_pet_data()->speed_mode == PET_MODE_FAST ? PET_FAST_TICK_MS : PET_NORMAL_TICK_MS);

        {
            pet_data_t *data = app_get_pet_data();
            float temp = dht11_get_temperature();
            float humidity = dht11_get_humidity();

            /* Skip if display is not active (during conversation) */
            if (!pet_display_is_active()) {
                continue;
            }

            /* Skip normal processing if in chart mode (timer handles chart rendering) */
            if (pet_display_is_chart_mode()) {
                continue;
            }

            /* Skip decay in pause mode */
            if (data->speed_mode == PET_MODE_PAUSE) {
                pet_display_render(data, temp, humidity);
                continue;
            }

            /* Apply decay */
            if (pet_attributes_decay(data)) {
                uint8_t old_mood = data->current_mood;
                data->current_mood = pet_attributes_calculate_mood(data);

                /* Check for death - emit signal to self to trigger transition */
                if (data->current_mood == PET_MOOD_DEAD) {
                    pet_sound_play(PET_SOUND_DEAD);
                    pet_display_render(data, temp, humidity);
                    ur_broadcast((ur_signal_t){
                        .id = SIG_PET_ATTR_CHANGED,
                        .src_id = ENT_ID_PET,
                        .ptr = data
                    });
                    /* Reset flow and signal death via return in rule handler */
                    ent->flow_line = 0;
                    ent->flags &= ~UR_FLAG_FLOW_RUNNING;
                    return STATE_PET_DEAD;
                }

                /* Play warning sounds for bad states */
                if (data->current_mood != old_mood) {
                    if (data->current_mood == PET_MOOD_HUNGRY) {
                        pet_sound_play(PET_SOUND_HUNGRY);
                    } else if (data->current_mood == PET_MOOD_TIRED) {
                        pet_sound_play(PET_SOUND_TIRED);
                    }

                    /* Broadcast mood change */
                    ur_broadcast((ur_signal_t){
                        .id = SIG_PET_MOOD_CHANGED,
                        .src_id = ENT_ID_PET,
                        .payload = { .u8 = { data->current_mood } }
                    });
                }

                /* Update display */
                pet_display_render(data, temp, humidity);

                /* Broadcast attribute change */
                ur_broadcast((ur_signal_t){
                    .id = SIG_PET_ATTR_CHANGED,
                    .src_id = ENT_ID_PET,
                    .ptr = data
                });
            }

            /* Check if save is needed */
            {
                uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
                if (now - data->last_save_ms >= PET_SAVE_INTERVAL_MS) {
                    pet_storage_save(data);
                    data->last_save_ms = now;
                }
            }
        }
    }

    UR_FLOW_END(ent);
}

/**
 * @brief Save flow - periodic NVS save
 */
static uint16_t save_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    pet_data_t *data = app_get_pet_data();
    pet_storage_save(data);
    data->last_save_ms = (uint32_t)(esp_timer_get_time() / 1000);
    return 0;
}

/**
 * @brief Handle button press
 */
static uint16_t on_button_pressed(ur_entity_t *ent, const ur_signal_t *sig)
{
    uint8_t button_id = sig->payload.u8[0];
    pet_data_t *data = app_get_pet_data();

    ESP_LOGI(TAG, "Button %d pressed", button_id);

    switch (button_id) {
        case BUTTON_FEED:
            pet_attributes_feed(data, false);
            pet_sound_play(PET_SOUND_FEED);
            break;

        case BUTTON_PLAY:
            pet_attributes_play(data);
            pet_sound_play(PET_SOUND_PLAY);
            break;

        case BUTTON_SLEEP:
            pet_attributes_sleep(data, false);
            pet_sound_play(PET_SOUND_SLEEP);
            break;

        case BUTTON_MODE:
            /* In chart mode: switch chart page */
            if (pet_display_is_chart_mode()) {
                pet_display_next_chart_page();
                pet_display_render_chart();
                return 0;
            }
            /* Cycle speed mode: NORMAL -> FAST -> PAUSE -> NORMAL */
            data->speed_mode = (data->speed_mode + 1) % 3;
            ESP_LOGI(TAG, "Speed mode: %d", data->speed_mode);
            break;

        default:
            return 0;
    }

    /* Update mood and display */
    data->current_mood = pet_attributes_calculate_mood(data);
    pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

    /* Broadcast attribute change */
    ur_broadcast((ur_signal_t){
        .id = SIG_PET_ATTR_CHANGED,
        .src_id = ENT_ID_PET,
        .ptr = data
    });

    return 0;
}

/**
 * @brief Handle button long press
 */
static uint16_t on_button_long_pressed(ur_entity_t *ent, const ur_signal_t *sig)
{
    uint8_t button_id = sig->payload.u8[0];
    pet_data_t *data = app_get_pet_data();

    ESP_LOGI(TAG, "Button %d long pressed", button_id);

    switch (button_id) {
        case BUTTON_FEED:
            /* Force feed */
            pet_attributes_feed(data, true);
            pet_sound_play(PET_SOUND_FEED);
            break;

        case BUTTON_SLEEP:
            /* Force sleep */
            pet_attributes_sleep(data, true);
            pet_sound_play(PET_SOUND_SLEEP);
            break;

        case BUTTON_MODE:
            /* If pet is dead, revive it */
            if (ent->current_state == STATE_PET_DEAD ||
                pet_attributes_is_dead(data)) {
                pet_attributes_revive(data);
                pet_sound_play(PET_SOUND_REVIVE);

                /* Update display */
                pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

                /* Broadcast attribute change */
                ur_broadcast((ur_signal_t){
                    .id = SIG_PET_ATTR_CHANGED,
                    .src_id = ENT_ID_PET,
                    .ptr = data
                });

                return STATE_PET_ACTIVE;
            }
            /* If pet is alive, toggle chart mode */
            pet_display_toggle_chart_mode();
            if (!pet_display_is_chart_mode()) {
                /* Exiting chart mode, refresh normal display */
                pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());
            } else {
                /* Entering chart mode, render chart immediately */
                pet_display_render_chart();
            }
            return 0;

        default:
            return 0;
    }

    /* Update mood and display */
    data->current_mood = pet_attributes_calculate_mood(data);
    pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

    /* Broadcast attribute change */
    ur_broadcast((ur_signal_t){
        .id = SIG_PET_ATTR_CHANGED,
        .src_id = ENT_ID_PET,
        .ptr = data
    });

    return 0;
}

/**
 * @brief Handle pet interaction (from TianShu or AI)
 */
static uint16_t on_pet_interaction(ur_entity_t *ent, const ur_signal_t *sig)
{
    pet_interaction_t interaction = (pet_interaction_t)sig->payload.u8[0];
    pet_data_t *data = app_get_pet_data();

    ESP_LOGI(TAG, "Pet interaction: %d", interaction);

    switch (interaction) {
        case PET_INTERACT_FEED:
            if (!pet_attributes_is_dead(data)) {
                pet_attributes_feed(data, false);
                pet_sound_play(PET_SOUND_FEED);
            }
            break;

        case PET_INTERACT_PLAY:
            if (!pet_attributes_is_dead(data)) {
                pet_attributes_play(data);
                pet_sound_play(PET_SOUND_PLAY);
            }
            break;

        case PET_INTERACT_SLEEP:
            if (!pet_attributes_is_dead(data)) {
                pet_attributes_sleep(data, false);
                pet_sound_play(PET_SOUND_SLEEP);
            }
            break;

        case PET_INTERACT_REVIVE:
            pet_attributes_revive(data);
            pet_sound_play(PET_SOUND_REVIVE);
            if (ent->current_state == STATE_PET_DEAD) {
                /* Update and transition */
                pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());
                ur_broadcast((ur_signal_t){
                    .id = SIG_PET_ATTR_CHANGED,
                    .src_id = ENT_ID_PET,
                    .ptr = data
                });
                return STATE_PET_ACTIVE;
            }
            break;
    }

    /* Update mood and display */
    data->current_mood = pet_attributes_calculate_mood(data);
    pet_display_render(data, dht11_get_temperature(), dht11_get_humidity());

    /* Broadcast attribute change */
    ur_broadcast((ur_signal_t){
        .id = SIG_PET_ATTR_CHANGED,
        .src_id = ENT_ID_PET,
        .ptr = data
    });

    return 0;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_pet_get(void)
{
    return &s_pet_entity;
}

ur_err_t ent_pet_init(void)
{
    esp_err_t err;
    pet_data_t *data = app_get_pet_data();

    /* Initialize pet storage and load data */
    pet_storage_init();
    if (pet_storage_load(data) != ESP_OK) {
        /* Use default values from app_buffers_init */
        ESP_LOGI(TAG, "Using default pet data");
    }

    /* Initialize attributes */
    pet_attributes_init(data);

    /* Initialize button driver */
    err = button_init(button_callback);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %d", err);
        return UR_ERR_INVALID_STATE;
    }

    /* Note: pet_sound_init() is deferred to on_active_entry()
     * because it calls baidu_tts_generate_prompt() which needs WiFi */

    /* Initialize pet display with shared u8g2 handle and mutex */
    u8g2_t *u8g2 = (u8g2_t *)ent_system_get_u8g2();
    void *oled_mutex = ent_system_get_oled_mutex();
    if (u8g2 && oled_mutex) {
        pet_display_init(u8g2, oled_mutex);
    } else {
        ESP_LOGW(TAG, "u8g2/mutex not available, pet display disabled");
    }

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_PET,
        .name = "pet",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_PET_INIT,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_pet_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init Pet entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Pet entity initialized");
    return UR_OK;
}
