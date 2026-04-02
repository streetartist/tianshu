/**
 * @file ent_asr.h
 * @brief Speech recognition entity
 *
 * Wraps Baidu ASR for speech-to-text conversion.
 */

#ifndef ENT_ASR_H
#define ENT_ASR_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ASR entity states
 */
typedef enum {
    STATE_ASR_IDLE = 1,         /**< Idle, waiting for request */
    STATE_ASR_PROCESSING,       /**< Processing audio */
} asr_state_t;

/**
 * @brief Get pointer to ASR entity
 */
ur_entity_t *ent_asr_get(void);

/**
 * @brief Initialize ASR entity
 * @return UR_OK on success
 */
ur_err_t ent_asr_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_ASR_H */
