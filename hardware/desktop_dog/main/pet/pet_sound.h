/**
 * @file pet_sound.h
 * @brief Pet sound feedback using TTS
 */

#ifndef PET_SOUND_H
#define PET_SOUND_H

#include "esp_err.h"
#include "app_payloads.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pet sound module
 * Pre-generates pet sound prompts if they don't exist.
 * @return ESP_OK on success
 */
esp_err_t pet_sound_init(void);

/**
 * @brief Play pet sound
 * @param sound Sound type to play
 * @return ESP_OK on success
 */
esp_err_t pet_sound_play(pet_sound_t sound);

/**
 * @brief Check if pet sound is currently playing
 * @return true if playing
 */
bool pet_sound_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif /* PET_SOUND_H */
