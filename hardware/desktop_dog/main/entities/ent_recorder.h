/**
 * @file ent_recorder.h
 * @brief Audio recording entity
 *
 * Records audio with VAD (Voice Activity Detection).
 */

#ifndef ENT_RECORDER_H
#define ENT_RECORDER_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Recorder entity states
 */
typedef enum {
    STATE_REC_IDLE = 1,     /**< Not recording */
    STATE_REC_RECORDING,    /**< Recording audio */
} recorder_state_t;

/**
 * @brief Get pointer to recorder entity
 */
ur_entity_t *ent_recorder_get(void);

/**
 * @brief Initialize recorder entity
 * @return UR_OK on success
 */
ur_err_t ent_recorder_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_RECORDER_H */
