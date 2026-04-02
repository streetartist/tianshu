/**
 * @file ent_system.c
 * @brief System coordinator entity implementation
 *
 * Manages the conversation flow:
 * READY -> LISTENING -> RECOGNIZING -> THINKING -> SPEAKING -> READY
 */

#include "ent_system.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"
#include "app_payloads.h"
#include "baidu_tts.h"
#include "pet/pet_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "u8g2.h"

#include <string.h>
#include <stdio.h>

static const char *TAG = "ENT_SYS";

/* ============================================================================
 * OLED Display
 * ========================================================================== */

static u8g2_t s_u8g2;
static SemaphoreHandle_t s_oled_mutex = NULL;

/* u8g2 I2C callback */
static uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg,
                                       uint8_t arg_int, void *arg_ptr)
{
    static i2c_cmd_handle_t handle;
    switch (msg) {
        case U8X8_MSG_BYTE_INIT:
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            handle = i2c_cmd_link_create();
            i2c_master_start(handle);
            i2c_master_write_byte(handle,
                (u8x8_GetI2CAddress(u8x8) << 1) | I2C_MASTER_WRITE, true);
            break;
        case U8X8_MSG_BYTE_SEND:
            i2c_master_write(handle, (uint8_t *)arg_ptr, arg_int, true);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            i2c_master_stop(handle);
            i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(100));
            i2c_cmd_link_delete(handle);
            break;
        default:
            break;
    }
    return 1;
}

static uint8_t u8g2_esp32_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg,
                                         uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;
        default:
            break;
    }
    return 1;
}

static void oled_init(void)
{
    s_oled_mutex = xSemaphoreCreateMutex();

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&s_u8g2, U8G2_R0,
        u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_delay_cb);
    u8x8_SetI2CAddress(&s_u8g2.u8x8, 0x3C);
    u8g2_InitDisplay(&s_u8g2);
    u8g2_SetPowerSave(&s_u8g2, 0);
    u8g2_ClearBuffer(&s_u8g2);
}

void ent_system_oled_show(const char *text)
{
    if (!s_oled_mutex) return;

    xSemaphoreTake(s_oled_mutex, portMAX_DELAY);

    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SetFont(&s_u8g2, u8g2_font_wqy12_t_gb2312);

    int y = 14;
    int line_height = 14;
    int max_width = 128;

    const char *p = text;
    char line[64];

    while (*p && y <= 64) {
        int line_len = 0;
        int line_width = 0;
        const char *start = p;

        /* Calculate how many chars fit on one line */
        while (*p && line_width < max_width) {
            int char_len = 1;
            /* UTF-8 Chinese chars are 3 bytes */
            if ((*p & 0xE0) == 0xE0) {
                char_len = 3;
            } else if ((*p & 0xC0) == 0xC0) {
                char_len = 2;
            }

            int char_width = (char_len == 3) ? 12 : 6;
            if (line_width + char_width > max_width) break;

            line_width += char_width;
            line_len += char_len;
            p += char_len;
        }

        if (line_len > 0 && line_len < sizeof(line)) {
            memcpy(line, start, line_len);
            line[line_len] = '\0';
            u8g2_DrawUTF8(&s_u8g2, 0, y, line);
            y += line_height;
        }
    }

    u8g2_SendBuffer(&s_u8g2);
    xSemaphoreGive(s_oled_mutex);
}

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_system_entity;

/* Forward declarations */
static uint16_t on_init_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_ready_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_listening_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_recognizing_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_thinking_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_speaking_entry(ur_entity_t *ent, const ur_signal_t *sig);

static uint16_t on_wake_detected(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_record_done(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_asr_result(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_asr_error(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_ai_response(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_ai_error(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_tts_done(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_tts_interrupted(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_display_text(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_ts_command(ur_entity_t *ent, const ur_signal_t *sig);

/* State: INIT */
static const ur_rule_t s_init_rules[] = {
    UR_RULE(SIG_SYSTEM_READY, STATE_SYS_READY, NULL),
    UR_RULE_END
};

/* State: READY */
static const ur_rule_t s_ready_rules[] = {
    UR_RULE(SIG_WAKE_DETECTED, STATE_SYS_LISTENING, on_wake_detected),
    UR_RULE(SIG_TS_COMMAND, 0, on_ts_command),
    UR_RULE(SIG_DISPLAY_TEXT, 0, on_display_text),
    UR_RULE_END
};

/* State: LISTENING */
static const ur_rule_t s_listening_rules[] = {
    UR_RULE(SIG_RECORD_DONE, STATE_SYS_RECOGNIZING, on_record_done),
    UR_RULE(SIG_RECORD_CANCEL, STATE_SYS_READY, NULL),
    UR_RULE(SIG_DISPLAY_TEXT, 0, on_display_text),
    UR_RULE_END
};

/* State: RECOGNIZING */
static const ur_rule_t s_recognizing_rules[] = {
    UR_RULE(SIG_ASR_RESULT, STATE_SYS_THINKING, on_asr_result),
    UR_RULE(SIG_ASR_ERROR, STATE_SYS_READY, on_asr_error),
    UR_RULE(SIG_DISPLAY_TEXT, 0, on_display_text),
    UR_RULE_END
};

/* State: THINKING */
static const ur_rule_t s_thinking_rules[] = {
    UR_RULE(SIG_AI_RESPONSE, STATE_SYS_SPEAKING, on_ai_response),
    UR_RULE(SIG_AI_ERROR, STATE_SYS_READY, on_ai_error),
    UR_RULE(SIG_DISPLAY_TEXT, 0, on_display_text),
    UR_RULE_END
};

/* State: SPEAKING */
static const ur_rule_t s_speaking_rules[] = {
    UR_RULE(SIG_TTS_DONE, STATE_SYS_READY, on_tts_done),
    UR_RULE(SIG_TTS_INTERRUPTED, STATE_SYS_LISTENING, on_tts_interrupted),
    UR_RULE(SIG_WAKE_DETECTED, 0, on_wake_detected),  /* For TTS interrupt */
    UR_RULE(SIG_DISPLAY_TEXT, 0, on_display_text),
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_SYS_INIT, 0, on_init_entry, NULL, s_init_rules),
    UR_STATE(STATE_SYS_READY, 0, on_ready_entry, NULL, s_ready_rules),
    UR_STATE(STATE_SYS_LISTENING, 0, on_listening_entry, NULL, s_listening_rules),
    UR_STATE(STATE_SYS_RECOGNIZING, 0, on_recognizing_entry, NULL, s_recognizing_rules),
    UR_STATE(STATE_SYS_THINKING, 0, on_thinking_entry, NULL, s_thinking_rules),
    UR_STATE(STATE_SYS_SPEAKING, 0, on_speaking_entry, NULL, s_speaking_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_init_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "System initializing...");
    ent_system_oled_show("Starting...");
    return 0;
}

static uint16_t on_ready_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "System ready, waiting for wake word");

    /* Re-enable pet display when returning to ready state */
    pet_display_set_active(true);

    /* Enable wake word detection */
    ur_emit_to_id(ENT_ID_WAKE_WORD,
        (ur_signal_t){ .id = SIG_WAKE_ENABLE, .src_id = ENT_ID_SYSTEM });

    return 0;
}

static uint16_t on_listening_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Listening...");
    ent_system_oled_show("Listening...");

    /* Start recording */
    ur_emit_to_id(ENT_ID_RECORDER,
        (ur_signal_t){ .id = SIG_RECORD_START, .src_id = ENT_ID_SYSTEM });

    return 0;
}

static uint16_t on_recognizing_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Recognizing...");
    ent_system_oled_show("识别中...");
    return 0;
}

static uint16_t on_thinking_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "AI thinking...");
    /* Don't overwrite OLED - keep showing ASR result */
    return 0;
}

static uint16_t on_speaking_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Speaking...");
    /* Don't overwrite OLED - keep showing AI response */
    return 0;
}

static uint16_t on_wake_detected(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Wake word detected!");

    /* Disable pet display during conversation */
    pet_display_set_active(false);

    /* If speaking, stop TTS */
    if (ent->current_state == STATE_SYS_SPEAKING) {
        ur_emit_to_id(ENT_ID_TTS,
            (ur_signal_t){ .id = SIG_TTS_STOP, .src_id = ENT_ID_SYSTEM });
        /* Will transition via SIG_TTS_INTERRUPTED */
        return 0;
    }

    /* Disable wake word during conversation */
    ur_emit_to_id(ENT_ID_WAKE_WORD,
        (ur_signal_t){ .id = SIG_WAKE_DISABLE, .src_id = ENT_ID_SYSTEM });

    /* Stop any playing audio */
    ur_emit_to_id(ENT_ID_AUDIO,
        (ur_signal_t){ .id = SIG_AUDIO_STOP, .src_id = ENT_ID_SYSTEM });

    return 0;  /* Transition handled by rule */
}

static uint16_t on_record_done(ur_entity_t *ent, const ur_signal_t *sig)
{
    audio_data_t *audio = (audio_data_t *)sig->ptr;
    ESP_LOGI(TAG, "Recording done: %d samples", audio->sample_count);

    /* Send to ASR */
    ur_emit_to_id(ENT_ID_ASR,
        (ur_signal_t){ .id = SIG_ASR_REQUEST, .src_id = ENT_ID_SYSTEM,
                       .ptr = audio });

    return 0;
}

static uint16_t on_asr_result(ur_entity_t *ent, const ur_signal_t *sig)
{
    char *text = (char *)sig->ptr;
    ESP_LOGI(TAG, "ASR result: %s", text);

    /* Display user's speech with prefix */
    char display_buf[128];
    snprintf(display_buf, sizeof(display_buf), "我: %s", text);
    ent_system_oled_show(display_buf);

    /* Send to AI */
    ur_emit_to_id(ENT_ID_AI,
        (ur_signal_t){ .id = SIG_AI_REQUEST, .src_id = ENT_ID_SYSTEM,
                       .ptr = text });

    return 0;
}

static uint16_t on_asr_error(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGW(TAG, "ASR error");
    ent_system_oled_show("ASR Error");
    return 0;
}

static uint16_t on_ai_response(ur_entity_t *ent, const ur_signal_t *sig)
{
    ai_response_t *resp = (ai_response_t *)sig->ptr;
    ESP_LOGI(TAG, "AI response: %s", resp->text);

    /* Display AI response with prefix */
    char display_buf[256];
    snprintf(display_buf, sizeof(display_buf), "AI: %s", resp->text);
    ent_system_oled_show(display_buf);

    /* Send to TTS */
    ur_emit_to_id(ENT_ID_TTS,
        (ur_signal_t){ .id = SIG_TTS_REQUEST, .src_id = ENT_ID_SYSTEM,
                       .ptr = resp->text });

    /* Re-enable wake word detection for interrupt */
    ur_emit_to_id(ENT_ID_WAKE_WORD,
        (ur_signal_t){ .id = SIG_WAKE_ENABLE, .src_id = ENT_ID_SYSTEM });

    return 0;
}

static uint16_t on_ai_error(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGW(TAG, "AI error");
    ent_system_oled_show("AI Error");
    return 0;
}

static uint16_t on_tts_done(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TTS done");

    ai_response_t *resp = app_get_ai_response();

    /* Display AI response after TTS finishes */
    char display_buf[256];
    snprintf(display_buf, sizeof(display_buf), "AI: %s", resp->text);
    ent_system_oled_show(display_buf);

    /* Check if we should continue conversation */
    if (resp->continue_chat) {
        ESP_LOGI(TAG, "Continue conversation");
        baidu_tts_play_prompt("continue.pcm");
        return STATE_SYS_LISTENING;
    }

    /* Execute pending actions (e.g., play music) */
    if (resp->action_type && resp->action_data) {
        if (strcmp(resp->action_type, "play_music") == 0) {
            ur_emit_to_id(ENT_ID_AUDIO,
                (ur_signal_t){ .id = SIG_AUDIO_PLAY, .src_id = ENT_ID_SYSTEM,
                               .ptr = resp->action_data });
        }
    }

    return 0;
}

static uint16_t on_tts_interrupted(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "TTS interrupted, starting new conversation");
    return 0;  /* Transition to LISTENING handled by rule */
}

static uint16_t on_display_text(ur_entity_t *ent, const ur_signal_t *sig)
{
    char *text = (char *)sig->ptr;
    if (text) {
        ent_system_oled_show(text);
    }
    return 0;
}

static uint16_t on_ts_command(ur_entity_t *ent, const ur_signal_t *sig)
{
    ts_command_t *cmd = (ts_command_t *)sig->ptr;
    ESP_LOGI(TAG, "TianShu command: %s", cmd->command);

    if (strcmp(cmd->command, "start_chat") == 0) {
        /* Trigger wake as if wake word was detected */
        ur_emit(ent, (ur_signal_t){ .id = SIG_WAKE_DETECTED,
                                     .src_id = ENT_ID_TIANSHU });
    }

    return 0;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_system_get(void)
{
    return &s_system_entity;
}

void *ent_system_get_u8g2(void)
{
    return &s_u8g2;
}

void *ent_system_get_oled_mutex(void)
{
    return s_oled_mutex;
}

ur_err_t ent_system_init(void)
{
    /* Initialize I2C */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    /* Initialize OLED */
    oled_init();

    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_SYSTEM,
        .name = "system",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_SYS_INIT,
        .user_data = NULL,
    };

    ur_err_t err = ur_init(&s_system_entity, &cfg);
    if (err != UR_OK) {
        ESP_LOGE(TAG, "Failed to init system entity: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "System entity initialized");
    return UR_OK;
}
