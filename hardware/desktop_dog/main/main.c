/**
 * @file main.c
 * @brief Desktop Dog - MicroReactor based IoT voice assistant
 *
 * Entry point for the application using MicroReactor framework.
 * Initializes all entities and runs the main dispatch loop.
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"

/* MicroReactor framework */
#include "ur_core.h"
#include "ur_flow.h"
#include "ur_trace.h"

/* Application headers */
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"

/* Entity headers */
#include "entities/ent_system.h"
#include "entities/ent_wake_word.h"
#include "entities/ent_recorder.h"
#include "entities/ent_asr.h"
#include "entities/ent_ai.h"
#include "entities/ent_tts.h"
#include "entities/ent_sensor.h"
#include "entities/ent_tianshu.h"
#include "entities/ent_audio.h"
#include "entities/ent_pet.h"

/* Drivers and services */
#include "wifi.h"
#include "i2s_mic.h"
#include "i2s_speaker.h"
#include "baidu_asr.h"
#include "baidu_tts.h"
#include "audio_player.h"

static const char *TAG = "MAIN";

/* Entity array for dispatch */
#define NUM_ENTITIES 10
static ur_entity_t *s_entities[NUM_ENTITIES];

/**
 * @brief Initialize all hardware peripherals
 */
static esp_err_t init_hardware(void)
{
    esp_err_t err;

    /* Show status via system entity's OLED */
    ent_system_oled_show("WiFi...");
    ESP_LOGI(TAG, "Connecting to WiFi...");
    err = wifi_init_sta(WIFI_SSID, WIFI_PASSWORD);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %d", err);
        return err;
    }
    ESP_LOGI(TAG, "WiFi connected");

    ent_system_oled_show("Mic init...");
    err = i2s_mic_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mic init failed: %d", err);
        return err;
    }

    ent_system_oled_show("Speaker init...");
    err = i2s_speaker_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Speaker init failed: %d", err);
        return err;
    }

    ent_system_oled_show("Audio init...");
    err = audio_player_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Audio player init failed: %d", err);
        return err;
    }

    /* Get Baidu token with retries */
    ent_system_oled_show("Get token...");
    char token[128] = {0};
    for (int retry = 0; retry < 5; retry++) {
        if (baidu_asr_get_token(token, sizeof(token)) == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "Get token failed, retry %d", retry + 1);
        ent_system_oled_show("Retry...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    /* Generate prompt audio files */
    ent_system_oled_show("Gen prompts...");
    baidu_tts_generate_prompt("在呢！", "wake.pcm");
    baidu_tts_generate_prompt("你可以继续说", "continue.pcm");

    return ESP_OK;
}

/**
 * @brief Initialize all MicroReactor entities
 */
static ur_err_t init_entities(void)
{
    ur_err_t err;

    ESP_LOGI(TAG, "Initializing entities...");

    /* System entity (must be first - initializes I2C/OLED) */
    err = ent_system_init();
    if (err != UR_OK) return err;
    s_entities[0] = ent_system_get();
    ur_register_entity(s_entities[0]);

    /* Wake word entity */
    ent_system_oled_show("Wake word...");
    err = ent_wake_word_init();
    if (err != UR_OK) return err;
    s_entities[1] = ent_wake_word_get();
    ur_register_entity(s_entities[1]);

    /* Recorder entity */
    err = ent_recorder_init();
    if (err != UR_OK) return err;
    s_entities[2] = ent_recorder_get();
    ur_register_entity(s_entities[2]);

    /* ASR entity */
    err = ent_asr_init();
    if (err != UR_OK) return err;
    s_entities[3] = ent_asr_get();
    ur_register_entity(s_entities[3]);

    /* AI entity */
    err = ent_ai_init();
    if (err != UR_OK) return err;
    s_entities[4] = ent_ai_get();
    ur_register_entity(s_entities[4]);

    /* TTS entity */
    err = ent_tts_init();
    if (err != UR_OK) return err;
    s_entities[5] = ent_tts_get();
    ur_register_entity(s_entities[5]);

    /* Sensor entity */
    ent_system_oled_show("Sensor init...");
    err = ent_sensor_init();
    if (err != UR_OK) return err;
    s_entities[6] = ent_sensor_get();
    ur_register_entity(s_entities[6]);

    /* TianShu entity */
    ent_system_oled_show("TianShu init...");
    err = ent_tianshu_init();
    if (err != UR_OK) return err;
    s_entities[7] = ent_tianshu_get();
    ur_register_entity(s_entities[7]);

    /* Audio playback entity */
    err = ent_audio_init();
    if (err != UR_OK) return err;
    s_entities[8] = ent_audio_get();
    ur_register_entity(s_entities[8]);

    /* Pet entity */
    ent_system_oled_show("Pet init...");
    err = ent_pet_init();
    if (err != UR_OK) return err;
    s_entities[9] = ent_pet_get();
    ur_register_entity(s_entities[9]);

    ESP_LOGI(TAG, "All entities initialized");

    /* Register entity names for trace output */
#if UR_CFG_TRACE_ENABLE
    ur_trace_register_entity_name(s_entities[0]->id, "System");
    ur_trace_register_entity_name(s_entities[1]->id, "WakeWord");
    ur_trace_register_entity_name(s_entities[2]->id, "Recorder");
    ur_trace_register_entity_name(s_entities[3]->id, "ASR");
    ur_trace_register_entity_name(s_entities[4]->id, "AI");
    ur_trace_register_entity_name(s_entities[5]->id, "TTS");
    ur_trace_register_entity_name(s_entities[6]->id, "Sensor");
    ur_trace_register_entity_name(s_entities[7]->id, "TianShu");
    ur_trace_register_entity_name(s_entities[8]->id, "Audio");
    ur_trace_register_entity_name(s_entities[9]->id, "Pet");

    /* Register signal names for trace output */
    ur_trace_register_signal_name(SIG_SYS_INIT, "SYS_INIT");
    ur_trace_register_signal_name(SIG_SYS_ENTRY, "SYS_ENTRY");
    ur_trace_register_signal_name(SIG_SYS_EXIT, "SYS_EXIT");
    ur_trace_register_signal_name(SIG_SYS_TIMEOUT, "SYS_TIMEOUT");
    ur_trace_register_signal_name(SIG_SYSTEM_READY, "SYSTEM_READY");
    ur_trace_register_signal_name(SIG_SYSTEM_ERROR, "SYSTEM_ERROR");
    ur_trace_register_signal_name(SIG_WAKE_DETECTED, "WAKE_DETECTED");
    ur_trace_register_signal_name(SIG_WAKE_ENABLE, "WAKE_ENABLE");
    ur_trace_register_signal_name(SIG_WAKE_DISABLE, "WAKE_DISABLE");
    ur_trace_register_signal_name(SIG_RECORD_START, "RECORD_START");
    ur_trace_register_signal_name(SIG_RECORD_DONE, "RECORD_DONE");
    ur_trace_register_signal_name(SIG_RECORD_CANCEL, "RECORD_CANCEL");
    ur_trace_register_signal_name(SIG_ASR_REQUEST, "ASR_REQUEST");
    ur_trace_register_signal_name(SIG_ASR_RESULT, "ASR_RESULT");
    ur_trace_register_signal_name(SIG_ASR_ERROR, "ASR_ERROR");
    ur_trace_register_signal_name(SIG_AI_REQUEST, "AI_REQUEST");
    ur_trace_register_signal_name(SIG_AI_RESPONSE, "AI_RESPONSE");
    ur_trace_register_signal_name(SIG_AI_ERROR, "AI_ERROR");
    ur_trace_register_signal_name(SIG_TTS_REQUEST, "TTS_REQUEST");
    ur_trace_register_signal_name(SIG_TTS_DONE, "TTS_DONE");
    ur_trace_register_signal_name(SIG_TTS_STOP, "TTS_STOP");
    ur_trace_register_signal_name(SIG_TTS_INTERRUPTED, "TTS_INTERRUPTED");
    ur_trace_register_signal_name(SIG_SENSOR_DATA, "SENSOR_DATA");
    ur_trace_register_signal_name(SIG_SENSOR_READ, "SENSOR_READ");
    ur_trace_register_signal_name(SIG_TS_COMMAND, "TS_COMMAND");
    ur_trace_register_signal_name(SIG_TS_HEARTBEAT, "TS_HEARTBEAT");
    ur_trace_register_signal_name(SIG_TS_REPORT, "TS_REPORT");
    ur_trace_register_signal_name(SIG_AUDIO_PLAY, "AUDIO_PLAY");
    ur_trace_register_signal_name(SIG_AUDIO_STOP, "AUDIO_STOP");
    ur_trace_register_signal_name(SIG_AUDIO_DONE, "AUDIO_DONE");
    ur_trace_register_signal_name(SIG_DISPLAY_TEXT, "DISPLAY_TEXT");
    ur_trace_register_signal_name(SIG_DISPLAY_CLEAR, "DISPLAY_CLEAR");
    ur_trace_register_signal_name(SIG_PET_TICK, "PET_TICK");
    ur_trace_register_signal_name(SIG_PET_SAVE, "PET_SAVE");
    ur_trace_register_signal_name(SIG_PET_ATTR_CHANGED, "PET_ATTR_CHANGED");
    ur_trace_register_signal_name(SIG_PET_INTERACTION, "PET_INTERACTION");
    ur_trace_register_signal_name(SIG_PET_MOOD_CHANGED, "PET_MOOD_CHANGED");
    ur_trace_register_signal_name(SIG_BTN_PRESSED, "BTN_PRESSED");
    ur_trace_register_signal_name(SIG_BTN_LONG_PRESSED, "BTN_LONG_PRESSED");
    ur_trace_register_signal_name(SIG_PET_SOUND_PLAY, "PET_SOUND_PLAY");
    ur_trace_register_signal_name(SIG_PET_SOUND_DONE, "PET_SOUND_DONE");

    /* Register state names for trace output */
    /* System entity states */
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_INIT, "Init");
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_READY, "Ready");
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_LISTENING, "Listening");
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_RECOGNIZING, "Recognizing");
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_THINKING, "Thinking");
    ur_trace_register_state_name(ENT_ID_SYSTEM, STATE_SYS_SPEAKING, "Speaking");

    /* WakeWord entity states */
    ur_trace_register_state_name(ENT_ID_WAKE_WORD, STATE_WAKE_DISABLED, "Disabled");
    ur_trace_register_state_name(ENT_ID_WAKE_WORD, STATE_WAKE_ENABLED, "Enabled");

    /* Recorder entity states */
    ur_trace_register_state_name(ENT_ID_RECORDER, STATE_REC_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_RECORDER, STATE_REC_RECORDING, "Recording");

    /* ASR entity states */
    ur_trace_register_state_name(ENT_ID_ASR, STATE_ASR_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_ASR, STATE_ASR_PROCESSING, "Processing");

    /* AI entity states */
    ur_trace_register_state_name(ENT_ID_AI, STATE_AI_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_AI, STATE_AI_PROCESSING, "Processing");

    /* TTS entity states */
    ur_trace_register_state_name(ENT_ID_TTS, STATE_TTS_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_TTS, STATE_TTS_SPEAKING, "Speaking");

    /* Sensor entity states */
    ur_trace_register_state_name(ENT_ID_SENSOR, STATE_SENSOR_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_SENSOR, STATE_SENSOR_READING, "Reading");

    /* TianShu entity states */
    ur_trace_register_state_name(ENT_ID_TIANSHU, STATE_TS_DISCONNECTED, "Disconnected");
    ur_trace_register_state_name(ENT_ID_TIANSHU, STATE_TS_CONNECTED, "Connected");

    /* Audio entity states */
    ur_trace_register_state_name(ENT_ID_AUDIO, STATE_AUDIO_IDLE, "Idle");
    ur_trace_register_state_name(ENT_ID_AUDIO, STATE_AUDIO_PLAYING, "Playing");

    /* Pet entity states */
    ur_trace_register_state_name(ENT_ID_PET, STATE_PET_INIT, "Init");
    ur_trace_register_state_name(ENT_ID_PET, STATE_PET_ACTIVE, "Active");
    ur_trace_register_state_name(ENT_ID_PET, STATE_PET_DEAD, "Dead");

    /* Send metadata to Scope */
    ur_trace_sync_metadata();
#endif

    return UR_OK;
}

/**
 * @brief Start all entities
 */
static void start_entities(void)
{
    ESP_LOGI(TAG, "Starting entities...");

    for (int i = 0; i < NUM_ENTITIES; i++) {
        ur_start(s_entities[i]);
    }

    /* Signal system ready */
    ur_broadcast((ur_signal_t){ .id = SIG_SYSTEM_READY, .src_id = 0 });

    ESP_LOGI(TAG, "All entities started");
}

/**
 * @brief Main dispatch loop task (tickless)
 *
 * Uses ur_run() which automatically:
 * 1. Processes all pending signals
 * 2. Sends SIG_SYS_TIMEOUT to flows waiting on UR_AWAIT_TIME
 * 3. Auto-starts flows that respond to SIG_SYS_TIMEOUT
 */
static void dispatch_task(void *arg)
{
    ESP_LOGI(TAG, "Dispatch loop started (tickless mode)");

#if UR_CFG_TRACE_ENABLE
    TickType_t last_sysinfo_time = xTaskGetTickCount();
    const TickType_t sysinfo_interval = pdMS_TO_TICKS(1000);
#endif

    while (1) {
        /* Single call handles everything */
        ur_run(s_entities, NUM_ENTITIES, DISPATCH_TIMEOUT_MS);

#if UR_CFG_TRACE_ENABLE
        TickType_t now = xTaskGetTickCount();
        if ((now - last_sysinfo_time) >= sysinfo_interval) {
            last_sysinfo_time = now;
            ur_trace_send_sysinfo();
        }
#endif
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Desktop Dog starting (MicroReactor)...");

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize static buffers */
    app_buffers_init();

    /* Initialize tracing (for MicroReactor Scope) */
#if UR_CFG_TRACE_ENABLE
    UR_TRACE_INIT();
    ur_trace_set_backend(&ur_trace_backend_uart);
    ur_trace_enable(true);
    ESP_LOGI(TAG, "Trace enabled (UART backend)");
#endif

    /* Initialize entities (system entity first for OLED) */
    ur_err_t err = init_entities();
    if (err != UR_OK) {
        ESP_LOGE(TAG, "Entity init failed: %d", err);
        return;
    }

    /* Initialize hardware */
    ret = init_hardware();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware init failed: %d", ret);
        ent_system_oled_show("Init failed!");
        return;
    }

    /* Start all entities */
    start_entities();

    ent_system_oled_show("Ready!");
    ESP_LOGI(TAG, "System ready!");

    /* Create dispatch task */
    xTaskCreatePinnedToCore(
        dispatch_task,
        "dispatch",
        8192,
        NULL,
        5,
        NULL,
        1  /* Run on core 1 */
    );

    /* app_main task can end - dispatch runs in its own task */
}
