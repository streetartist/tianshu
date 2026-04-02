/**
 * @file pet_storage.h
 * @brief Pet data NVS storage
 */

#ifndef PET_STORAGE_H
#define PET_STORAGE_H

#include "esp_err.h"
#include "app_payloads.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pet storage
 * @return ESP_OK on success
 */
esp_err_t pet_storage_init(void);

/**
 * @brief Load pet data from NVS
 * @param data Pointer to pet data structure to fill
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no data exists
 */
esp_err_t pet_storage_load(pet_data_t *data);

/**
 * @brief Save pet data to NVS
 * @param data Pointer to pet data structure to save
 * @return ESP_OK on success
 */
esp_err_t pet_storage_save(const pet_data_t *data);

/**
 * @brief Clear pet data from NVS
 * @return ESP_OK on success
 */
esp_err_t pet_storage_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* PET_STORAGE_H */
