/**
 * @file app_config.h
 * @brief Desktop Dog application configuration
 *
 * Hardware pins, entity IDs, network settings, and other constants.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Entity IDs
 * ========================================================================== */

#define ENT_ID_SYSTEM       1   /**< System coordinator entity */
#define ENT_ID_WAKE_WORD    2   /**< Wake word detection entity */
#define ENT_ID_RECORDER     3   /**< Audio recording entity */
#define ENT_ID_ASR          4   /**< Speech recognition entity */
#define ENT_ID_AI           5   /**< AI conversation entity */
#define ENT_ID_TTS          6   /**< Text-to-speech entity */
#define ENT_ID_SENSOR       7   /**< DHT11 sensor entity */
#define ENT_ID_TIANSHU      8   /**< TianShu IoT platform entity */
#define ENT_ID_AUDIO        9   /**< Music/audio playback entity */
#define ENT_ID_PET          10  /**< Pet virtual pet entity */

/* ============================================================================
 * Hardware Configuration
 * ========================================================================== */

/* I2C for OLED */
#define I2C_MASTER_SCL_IO       9
#define I2C_MASTER_SDA_IO       8
#define I2C_MASTER_NUM          0   /* I2C_NUM_0 */
#define I2C_MASTER_FREQ_HZ      400000

/* DHT11 */
#define DHT11_GPIO              45

/* Pet Buttons */
#define BUTTON_1_GPIO           38  /**< BTN1: Feed */
#define BUTTON_2_GPIO           39  /**< BTN2: Play */
#define BUTTON_3_GPIO           40  /**< BTN3: Sleep */
#define BUTTON_4_GPIO           41  /**< BTN4: Mode/Revive */
#define BUTTON_DEBOUNCE_MS      50  /**< Button debounce time */
#define BUTTON_LONG_PRESS_MS    1000 /**< Long press threshold */

/* ============================================================================
 * Network Configuration
 * ========================================================================== */

#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"

/* TianShu IoT Platform */
#define TIANSHU_SERVER          "https://your-tianshu-server.com"
#define TIANSHU_DEVICE_ID       "YOUR_DEVICE_ID"
#define TIANSHU_SECRET_KEY      "YOUR_SECRET_KEY"
#define TIANSHU_HEARTBEAT_SEC   30
#define TIANSHU_REPORT_SEC      10

/* ============================================================================
 * Audio Configuration
 * ========================================================================== */

#define AUDIO_SAMPLE_RATE       16000
#define AUDIO_MAX_RECORD_SEC    5
#define AUDIO_BUFFER_SAMPLES    (AUDIO_SAMPLE_RATE * AUDIO_MAX_RECORD_SEC)

/* VAD (Voice Activity Detection) Parameters */
#define VAD_FRAME_SIZE          512     /**< Samples per frame */
#define VAD_THRESHOLD           500     /**< Energy threshold */
#define VAD_SILENCE_FRAMES      30      /**< ~1 second of silence to stop */
#define VAD_MIN_FRAMES          16      /**< ~0.5 second minimum recording */

/* ============================================================================
 * Buffer Sizes
 * ========================================================================== */

#define ASR_RESULT_SIZE         256     /**< Max ASR result text length */
#define AI_ANSWER_SIZE          2048    /**< Max AI answer text length */
#define DISPLAY_TEXT_SIZE       128     /**< Max display text length */

/* ============================================================================
 * Timing Configuration
 * ========================================================================== */

#define WAKE_DETECT_INTERVAL_MS 10      /**< Wake word detection interval */
#define SENSOR_READ_INTERVAL_MS 10000   /**< DHT11 read interval (10 sec) */
#define DISPATCH_TIMEOUT_MS     10      /**< Main loop dispatch timeout */

/* Pet timing */
#define PET_NORMAL_TICK_MS      30000   /**< Normal mode: attribute decay every 30s */
#define PET_FAST_TICK_MS        1000    /**< Fast mode: attribute decay every 1s */
#define PET_SAVE_INTERVAL_MS    60000   /**< Auto-save to NVS every 60s */

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
