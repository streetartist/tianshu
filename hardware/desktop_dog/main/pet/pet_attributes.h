/**
 * @file pet_attributes.h
 * @brief Pet attribute management and decay logic
 */

#ifndef PET_ATTRIBUTES_H
#define PET_ATTRIBUTES_H

#include <stdint.h>
#include <stdbool.h>
#include "app_payloads.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pet attributes module
 * @param data Pointer to pet data structure
 */
void pet_attributes_init(pet_data_t *data);

/**
 * @brief Apply attribute decay (called on each tick)
 * @param data Pointer to pet data structure
 * @return true if any attribute changed
 */
bool pet_attributes_decay(pet_data_t *data);

/**
 * @brief Feed the pet
 * @param data Pointer to pet data structure
 * @param force Force feed even if not hungry
 */
void pet_attributes_feed(pet_data_t *data, bool force);

/**
 * @brief Play with the pet
 * @param data Pointer to pet data structure
 */
void pet_attributes_play(pet_data_t *data);

/**
 * @brief Let the pet rest
 * @param data Pointer to pet data structure
 * @param force Force sleep
 */
void pet_attributes_sleep(pet_data_t *data, bool force);

/**
 * @brief Revive a dead pet
 * @param data Pointer to pet data structure
 */
void pet_attributes_revive(pet_data_t *data);

/**
 * @brief Calculate current mood based on attributes
 * @param data Pointer to pet data structure
 * @return Calculated mood (pet_mood_t)
 */
pet_mood_t pet_attributes_calculate_mood(const pet_data_t *data);

/**
 * @brief Check if pet is dead
 * @param data Pointer to pet data structure
 * @return true if pet is dead
 */
bool pet_attributes_is_dead(const pet_data_t *data);

/**
 * @brief Get mood name string
 * @param mood Mood value
 * @return Mood name string
 */
const char *pet_attributes_mood_name(pet_mood_t mood);

#ifdef __cplusplus
}
#endif

#endif /* PET_ATTRIBUTES_H */
