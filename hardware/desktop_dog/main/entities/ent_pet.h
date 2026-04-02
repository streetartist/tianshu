/**
 * @file ent_pet.h
 * @brief Pet entity - virtual pet state machine
 */

#ifndef ENT_PET_H
#define ENT_PET_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pet entity states
 */
typedef enum {
    STATE_PET_INIT = 1,     /**< Initializing */
    STATE_PET_ACTIVE,       /**< Active and running */
    STATE_PET_DEAD,         /**< Pet is dead */
} pet_state_t;

/**
 * @brief Get pointer to Pet entity
 */
ur_entity_t *ent_pet_get(void);

/**
 * @brief Initialize Pet entity
 * @return UR_OK on success
 */
ur_err_t ent_pet_init(void);

/**
 * @brief Get u8g2 handle for pet display
 * This is used to share the OLED with pet_display module
 */
void *ent_pet_get_u8g2(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_PET_H */
