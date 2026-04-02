/**
 * @file ent_system.h
 * @brief System coordinator entity
 *
 * Central coordinator that manages the overall conversation flow
 * and handles OLED display updates.
 */

#ifndef ENT_SYSTEM_H
#define ENT_SYSTEM_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System entity states
 */
typedef enum {
    STATE_SYS_INIT = 1,         /**< Initializing */
    STATE_SYS_READY,            /**< Ready, waiting for wake word */
    STATE_SYS_LISTENING,        /**< Recording user speech */
    STATE_SYS_RECOGNIZING,      /**< ASR processing */
    STATE_SYS_THINKING,         /**< AI processing */
    STATE_SYS_SPEAKING,         /**< TTS playback */
} system_state_t;

/**
 * @brief Get pointer to system entity
 */
ur_entity_t *ent_system_get(void);

/**
 * @brief Initialize system entity
 *
 * Also initializes hardware: I2C, OLED, etc.
 *
 * @return UR_OK on success
 */
ur_err_t ent_system_init(void);

/**
 * @brief Show text on OLED display
 *
 * Thread-safe wrapper for OLED display.
 *
 * @param text Text to display (UTF-8 supported)
 */
void ent_system_oled_show(const char *text);

/**
 * @brief Get pointer to u8g2 display handle
 * Used by pet_display to share the OLED.
 */
void *ent_system_get_u8g2(void);

/**
 * @brief Get OLED mutex for shared access
 * Used by pet_display to coordinate OLED access.
 */
void *ent_system_get_oled_mutex(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_SYSTEM_H */
