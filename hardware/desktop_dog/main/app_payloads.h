/**
 * @file app_payloads.h
 * @brief Desktop Dog payload structure definitions
 *
 * Data structures passed via signal pointers between entities.
 */

#ifndef APP_PAYLOADS_H
#define APP_PAYLOADS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio data payload for ASR/recording
 */
typedef struct {
    int16_t *samples;       /**< Pointer to audio samples buffer */
    size_t sample_count;    /**< Number of samples */
    uint32_t sample_rate;   /**< Sample rate in Hz */
} audio_data_t;

/**
 * @brief AI response payload
 */
typedef struct {
    char *text;             /**< Response text */
    bool has_tool_call;     /**< Whether response includes tool call */
    bool continue_chat;     /**< Whether to continue conversation */
    char *action_type;      /**< Pending action type (e.g., "play_music") */
    char *action_data;      /**< Pending action data (e.g., music URL) */
} ai_response_t;

/**
 * @brief Sensor data payload for DHT11
 */
typedef struct {
    float temperature;      /**< Temperature in Celsius */
    float humidity;         /**< Relative humidity in percent */
    bool valid;             /**< Whether reading is valid */
} sensor_data_t;

/**
 * @brief TianShu command payload
 */
typedef struct {
    char *command;          /**< Command name */
    char *params;           /**< Command parameters (JSON string) */
} ts_command_t;

/**
 * @brief Pet mood types
 */
typedef enum {
    PET_MOOD_HAPPY = 0,     /**< happiness > 70 */
    PET_MOOD_NORMAL,        /**< Balanced state */
    PET_MOOD_SAD,           /**< happiness < 30 */
    PET_MOOD_HUNGRY,        /**< hunger > 70 */
    PET_MOOD_TIRED,         /**< fatigue > 70 */
    PET_MOOD_SICK,          /**< Multiple attributes abnormal */
    PET_MOOD_DEAD,          /**< Any attribute at extreme */
} pet_mood_t;

/**
 * @brief Pet interaction types
 */
typedef enum {
    PET_INTERACT_FEED = 0,  /**< Feed the pet */
    PET_INTERACT_PLAY,      /**< Play with the pet */
    PET_INTERACT_SLEEP,     /**< Let pet rest */
    PET_INTERACT_REVIVE,    /**< Revive dead pet */
} pet_interaction_t;

/**
 * @brief Pet sound types
 */
typedef enum {
    PET_SOUND_FEED = 0,     /**< "好吃！" */
    PET_SOUND_PLAY,         /**< "真开心！" */
    PET_SOUND_SLEEP,        /**< "晚安~" */
    PET_SOUND_HUNGRY,       /**< "我饿了..." */
    PET_SOUND_TIRED,        /**< "好困..." */
    PET_SOUND_DEAD,         /**< "我不行了..." */
    PET_SOUND_REVIVE,       /**< "我又活过来了！" */
} pet_sound_t;

/**
 * @brief Pet statistics
 */
typedef struct {
    uint8_t happiness;      /**< Happiness 0-100 (100=very happy) */
    uint8_t hunger;         /**< Hunger 0-100 (0=full, 100=starving) */
    uint8_t fatigue;        /**< Fatigue 0-100 (0=energetic, 100=exhausted) */
} pet_stats_t;

/**
 * @brief Pet speed mode
 */
typedef enum {
    PET_MODE_NORMAL = 0,    /**< Normal mode: 30s tick */
    PET_MODE_FAST,          /**< Fast mode: 1s tick */
    PET_MODE_PAUSE,         /**< Pause mode: no decay */
} pet_speed_mode_t;

/**
 * @brief Pet data payload
 */
typedef struct {
    pet_stats_t stats;      /**< Current pet statistics */
    uint8_t current_mood;   /**< Current mood (pet_mood_t) */
    uint8_t speed_mode;     /**< Speed mode (pet_speed_mode_t) */
    uint32_t last_save_ms;  /**< Last save timestamp */
} pet_data_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_PAYLOADS_H */
