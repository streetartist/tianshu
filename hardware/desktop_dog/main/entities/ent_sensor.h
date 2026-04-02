/**
 * @file ent_sensor.h
 * @brief DHT11 temperature/humidity sensor entity
 */

#ifndef ENT_SENSOR_H
#define ENT_SENSOR_H

#include "ur_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensor entity states
 */
typedef enum {
    STATE_SENSOR_IDLE = 1,      /**< Idle, waiting for next read cycle */
    STATE_SENSOR_READING,       /**< Reading sensor data */
} sensor_state_t;

/**
 * @brief Get pointer to sensor entity
 */
ur_entity_t *ent_sensor_get(void);

/**
 * @brief Initialize sensor entity
 * @return UR_OK on success
 */
ur_err_t ent_sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENT_SENSOR_H */
