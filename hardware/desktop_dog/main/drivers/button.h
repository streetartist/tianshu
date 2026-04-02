/**
 * @file button.h
 * @brief Button driver with debounce and long press detection
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button index definitions
 */
typedef enum {
    BUTTON_FEED = 0,    /**< BTN1: Feed (GPIO38) */
    BUTTON_PLAY,        /**< BTN2: Play (GPIO39) */
    BUTTON_SLEEP,       /**< BTN3: Sleep (GPIO40) */
    BUTTON_MODE,        /**< BTN4: Mode/Revive (GPIO41) */
    BUTTON_COUNT
} button_id_t;

/**
 * @brief Button event callback type
 * @param button_id Button index (0-3)
 * @param long_press true if long press, false if short press
 */
typedef void (*button_event_cb_t)(uint8_t button_id, bool long_press);

/**
 * @brief Initialize button driver
 * @param callback Event callback function
 * @return ESP_OK on success
 */
esp_err_t button_init(button_event_cb_t callback);

/**
 * @brief Deinitialize button driver
 */
void button_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
