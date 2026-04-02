/**
 * @file app_signals.h
 * @brief Desktop Dog application signal definitions
 *
 * All user-defined signals for inter-entity communication.
 * Based on MicroReactor framework.
 */

#ifndef APP_SIGNALS_H
#define APP_SIGNALS_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application signal definitions
 *
 * Signal ranges:
 * - System signals:   SIG_USER_BASE + 0-9
 * - Wake word:        SIG_USER_BASE + 10-19
 * - Recording:        SIG_USER_BASE + 20-29
 * - ASR:              SIG_USER_BASE + 30-39
 * - AI:               SIG_USER_BASE + 40-49
 * - TTS:              SIG_USER_BASE + 50-59
 * - Sensor:           SIG_USER_BASE + 60-69
 * - TianShu:          SIG_USER_BASE + 70-79
 * - Audio playback:   SIG_USER_BASE + 80-89
 * - Display:          SIG_USER_BASE + 90-99
 * - Pet:              SIG_USER_BASE + 100-109
 * - Button:           SIG_USER_BASE + 110-119
 * - Pet Sound:        SIG_USER_BASE + 120-129
 */
typedef enum {
    /* System signals (0-9) */
    SIG_SYSTEM_READY = SIG_USER_BASE,       /**< System initialization complete */
    SIG_SYSTEM_ERROR,                        /**< System error occurred */

    /* Wake word signals (10-19) */
    SIG_WAKE_DETECTED = SIG_USER_BASE + 10, /**< Wake word detected */
    SIG_WAKE_ENABLE,                         /**< Enable wake word detection */
    SIG_WAKE_DISABLE,                        /**< Disable wake word detection */

    /* Recording signals (20-29) */
    SIG_RECORD_START = SIG_USER_BASE + 20,  /**< Start recording audio */
    SIG_RECORD_DONE,                         /**< Recording complete, ptr -> audio_data_t* */
    SIG_RECORD_CANCEL,                       /**< Cancel current recording */
    SIG_MIC_DATA_READY,                      /**< Mic data available in pipe */

    /* ASR signals (30-39) */
    SIG_ASR_REQUEST = SIG_USER_BASE + 30,   /**< Request ASR, ptr -> audio_data_t* */
    SIG_ASR_RESULT,                          /**< ASR result, ptr -> char* text */
    SIG_ASR_ERROR,                           /**< ASR error occurred */

    /* AI signals (40-49) */
    SIG_AI_REQUEST = SIG_USER_BASE + 40,    /**< Request AI chat, ptr -> char* question */
    SIG_AI_RESPONSE,                         /**< AI response, ptr -> ai_response_t* */
    SIG_AI_ERROR,                            /**< AI error occurred */

    /* TTS signals (50-59) */
    SIG_TTS_REQUEST = SIG_USER_BASE + 50,   /**< Request TTS, ptr -> char* text */
    SIG_TTS_DONE,                            /**< TTS playback complete */
    SIG_TTS_STOP,                            /**< Stop TTS playback */
    SIG_TTS_INTERRUPTED,                     /**< TTS was interrupted */
    SIG_TTS_PLAYBACK_DONE,                   /**< Internal: TTS playback finished (from baidu_tts task) */

    /* Sensor signals (60-69) */
    SIG_SENSOR_DATA = SIG_USER_BASE + 60,   /**< Sensor data available, payload.f32 -> temp */
    SIG_SENSOR_READ,                         /**< Request sensor reading */

    /* TianShu signals (70-79) */
    SIG_TS_COMMAND = SIG_USER_BASE + 70,    /**< Remote command received, ptr -> command string */
    SIG_TS_HEARTBEAT,                        /**< Heartbeat timer tick */
    SIG_TS_REPORT,                           /**< Request data report */

    /* Audio playback signals (80-89) */
    SIG_AUDIO_PLAY = SIG_USER_BASE + 80,    /**< Play audio, ptr -> url string */
    SIG_AUDIO_STOP,                          /**< Stop audio playback */
    SIG_AUDIO_DONE,                          /**< Audio playback complete */

    /* Display signals (90-99) */
    SIG_DISPLAY_TEXT = SIG_USER_BASE + 90,  /**< Display text, ptr -> char* text */
    SIG_DISPLAY_CLEAR,                       /**< Clear display */

    /* Pet signals (100-109) */
    SIG_PET_TICK = SIG_USER_BASE + 100,     /**< Pet attribute decay tick */
    SIG_PET_SAVE,                            /**< Save pet data to NVS */
    SIG_PET_ATTR_CHANGED,                    /**< Pet attributes changed, ptr -> pet_data_t* */
    SIG_PET_INTERACTION,                     /**< Pet interaction event, payload.u8 = interaction type */
    SIG_PET_MOOD_CHANGED,                    /**< Pet mood changed, payload.u8 = new mood */

    /* Button signals (110-119) */
    SIG_BTN_PRESSED = SIG_USER_BASE + 110,  /**< Button pressed, payload.u8 = button index (0-3) */
    SIG_BTN_LONG_PRESSED,                    /**< Button long pressed, payload.u8 = button index */

    /* Pet sound signals (120-129) */
    SIG_PET_SOUND_PLAY = SIG_USER_BASE + 120, /**< Play pet sound, payload.u8 = sound type */
    SIG_PET_SOUND_DONE,                       /**< Pet sound playback complete */

} app_signal_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_SIGNALS_H */
