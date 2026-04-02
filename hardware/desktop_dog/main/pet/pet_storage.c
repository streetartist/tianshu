/**
 * @file pet_storage.c
 * @brief Pet data NVS storage
 */

#include "pet_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PET_STOR";

#define NVS_NAMESPACE "pet_data"
#define NVS_KEY_STATS "stats"

/* Storage structure for NVS (packed) */
typedef struct __attribute__((packed)) {
    uint8_t happiness;
    uint8_t hunger;
    uint8_t fatigue;
    uint8_t speed_mode;
} pet_nvs_data_t;

esp_err_t pet_storage_init(void)
{
    /* NVS should already be initialized by main.c */
    ESP_LOGI(TAG, "Pet storage initialized");
    return ESP_OK;
}

esp_err_t pet_storage_load(pet_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved pet data found, using defaults");
        } else {
            ESP_LOGW(TAG, "NVS open failed: %d", err);
        }
        return err;
    }

    pet_nvs_data_t nvs_data;
    size_t size = sizeof(nvs_data);

    err = nvs_get_blob(handle, NVS_KEY_STATS, &nvs_data, &size);
    nvs_close(handle);

    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved pet data found");
        } else {
            ESP_LOGW(TAG, "NVS read failed: %d", err);
        }
        return err;
    }

    /* Copy to pet_data structure */
    data->stats.happiness = nvs_data.happiness;
    data->stats.hunger = nvs_data.hunger;
    data->stats.fatigue = nvs_data.fatigue;
    data->speed_mode = nvs_data.speed_mode;

    ESP_LOGI(TAG, "Pet data loaded: H=%d, Hu=%d, F=%d, Mode=%d",
             data->stats.happiness, data->stats.hunger,
             data->stats.fatigue, data->speed_mode);

    return ESP_OK;
}

esp_err_t pet_storage_save(const pet_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %d", err);
        return err;
    }

    pet_nvs_data_t nvs_data = {
        .happiness = data->stats.happiness,
        .hunger = data->stats.hunger,
        .fatigue = data->stats.fatigue,
        .speed_mode = data->speed_mode
    };

    err = nvs_set_blob(handle, NVS_KEY_STATS, &nvs_data, sizeof(nvs_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS write failed: %d", err);
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "Pet data saved: H=%d, Hu=%d, F=%d, Mode=%d",
             data->stats.happiness, data->stats.hunger,
             data->stats.fatigue, data->speed_mode);

    return ESP_OK;
}

esp_err_t pet_storage_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_key(handle, NVS_KEY_STATS);
    if (err == ESP_OK) {
        nvs_commit(handle);
    }

    nvs_close(handle);

    ESP_LOGI(TAG, "Pet data cleared");
    return ESP_OK;
}
