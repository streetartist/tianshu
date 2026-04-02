/**
 * @file app_buffers.h
 * @brief Desktop Dog static buffer declarations
 *
 * All buffers are statically allocated to avoid dynamic memory allocation.
 */

#ifndef APP_BUFFERS_H
#define APP_BUFFERS_H

#include <stdint.h>
#include "app_config.h"
#include "app_payloads.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all static buffers
 *
 * Must be called early in app_main() before any entities start.
 */
void app_buffers_init(void);

/**
 * @brief Get pointer to audio sample buffer (in PSRAM)
 */
int16_t *app_get_audio_buffer(void);

/**
 * @brief Get pointer to ASR result buffer
 */
char *app_get_asr_result_buffer(void);

/**
 * @brief Get pointer to AI answer buffer
 */
char *app_get_ai_answer_buffer(void);

/**
 * @brief Get pointer to audio_data_t structure
 */
audio_data_t *app_get_audio_data(void);

/**
 * @brief Get pointer to ai_response_t structure
 */
ai_response_t *app_get_ai_response(void);

/**
 * @brief Get pointer to sensor_data_t structure
 */
sensor_data_t *app_get_sensor_data(void);

/**
 * @brief Get pointer to display text buffer
 */
char *app_get_display_buffer(void);

/**
 * @brief Get pointer to pet_data_t structure
 */
pet_data_t *app_get_pet_data(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BUFFERS_H */
