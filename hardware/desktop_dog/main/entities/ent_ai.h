/**
 * @file ent_ai.h
 * @brief AI conversation entity
 *
 * Wraps Mimo AI for chat responses and tool calling.
 */

#ifndef ENT_AI_H
#define ENT_AI_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AI entity states
 */
typedef enum {
    STATE_AI_IDLE = 1,          /**< Idle, waiting for request */
    STATE_AI_PROCESSING,        /**< Processing question */
} ai_state_t;

/**
 * @brief Get pointer to AI entity
 */
ur_entity_t *ent_ai_get(void);

/**
 * @brief Initialize AI entity
 * @return UR_OK on success
 */
ur_err_t ent_ai_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_AI_H */
