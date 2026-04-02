/**
 * @file ent_tianshu.h
 * @brief TianShu IoT platform entity
 */

#ifndef ENT_TIANSHU_H
#define ENT_TIANSHU_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TianShu entity states
 */
typedef enum {
    STATE_TS_DISCONNECTED = 1,  /**< Not connected */
    STATE_TS_CONNECTED,         /**< Connected and running */
} tianshu_state_t;

/**
 * @brief Get pointer to TianShu entity
 */
ur_entity_t *ent_tianshu_get(void);

/**
 * @brief Initialize TianShu entity
 * @return UR_OK on success
 */
ur_err_t ent_tianshu_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_TIANSHU_H */
