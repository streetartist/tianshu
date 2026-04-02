/**
 * @file ent_audio.h
 * @brief Audio/music playback entity
 */

#ifndef ENT_AUDIO_H
#define ENT_AUDIO_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio entity states
 */
typedef enum {
    STATE_AUDIO_IDLE = 1,       /**< Not playing */
    STATE_AUDIO_PLAYING,        /**< Playing audio */
} audio_state_t;

/**
 * @brief Get pointer to audio entity
 */
ur_entity_t *ent_audio_get(void);

/**
 * @brief Initialize audio entity
 * @return UR_OK on success
 */
ur_err_t ent_audio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_AUDIO_H */
