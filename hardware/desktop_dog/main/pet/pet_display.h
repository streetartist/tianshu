/**
 * @file pet_display.h
 * @brief Pet OLED expression display
 */

#ifndef PET_DISPLAY_H
#define PET_DISPLAY_H

#include "esp_err.h"
#include "app_payloads.h"
#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pet display module
 * @param u8g2 Pointer to u8g2 display handle (shared with ent_system)
 * @param oled_mutex Shared OLED mutex from ent_system
 * @return ESP_OK on success
 */
esp_err_t pet_display_init(u8g2_t *u8g2, void *oled_mutex);

/**
 * @brief Render pet display with current status
 * @param data Pointer to pet data
 * @param temp Current temperature
 * @param humidity Current humidity
 */
void pet_display_render(const pet_data_t *data, float temp, float humidity);

/**
 * @brief Check if pet display is currently active
 * @return true if pet display is active
 */
bool pet_display_is_active(void);

/**
 * @brief Enable/disable pet display
 * @param enable true to enable, false to disable
 */
void pet_display_set_active(bool enable);

/**
 * @brief Check if chart mode is active
 * @return true if in chart mode
 */
bool pet_display_is_chart_mode(void);

/**
 * @brief Toggle chart mode (show data curves)
 */
void pet_display_toggle_chart_mode(void);

/**
 * @brief Switch to next chart page (cycles through pages)
 */
void pet_display_next_chart_page(void);

/**
 * @brief Record data point for chart history
 * @param data Pet data
 * @param temp Temperature
 * @param humidity Humidity
 */
void pet_display_record_data(const pet_data_t *data, float temp, float humidity);

/**
 * @brief Render chart page with data curves
 */
void pet_display_render_chart(void);

#ifdef __cplusplus
}
#endif

#endif /* PET_DISPLAY_H */
