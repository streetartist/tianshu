/**
 * @file ent_tts.h
 * @brief Text-to-speech entity
 *
 * Wraps Baidu TTS with interrupt support.
 */

#ifndef ENT_TTS_H
#define ENT_TTS_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TTS entity states
 */
typedef enum {
    STATE_TTS_IDLE = 1,         /**< Idle, waiting for request */
    STATE_TTS_SPEAKING,         /**< Speaking (TTS playing) */
} tts_state_t;

/**
 * @brief Get pointer to TTS entity
 */
ur_entity_t *ent_tts_get(void);

/**
 * @brief Initialize TTS entity
 * @return UR_OK on success
 */
ur_err_t ent_tts_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_TTS_H */
