/**
 * @file ent_wake_word.h
 * @brief Wake word detection entity
 *
 * Continuously monitors audio for wake word "Hi,Lexin".
 */

#ifndef ENT_WAKE_WORD_H
#define ENT_WAKE_WORD_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wake word entity states
 */
typedef enum {
    STATE_WAKE_DISABLED = 1,    /**< Detection disabled */
    STATE_WAKE_ENABLED,         /**< Detection enabled */
} wake_state_t;

/**
 * @brief Get pointer to wake word entity
 */
ur_entity_t *ent_wake_word_get(void);

/**
 * @brief Initialize wake word entity
 * @return UR_OK on success
 */
ur_err_t ent_wake_word_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_WAKE_WORD_H */
