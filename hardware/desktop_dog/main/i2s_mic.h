#pragma once

#include "esp_err.h"
#include "driver/i2s_std.h"
#include <stddef.h>
#include <stdint.h>

// I2S通道句柄
extern i2s_chan_handle_t g_tx_handle;
extern i2s_chan_handle_t g_rx_handle;

/**
 * @brief Initialize I2S microphone
 */
esp_err_t i2s_mic_init(void);

/**
 * @brief Read samples from mic (blocking, legacy API)
 */
size_t i2s_mic_read(int16_t *buffer, size_t samples);

/**
 * @brief Disable microphone
 */
esp_err_t i2s_mic_disable(void);

/**
 * @brief Enable microphone
 */
esp_err_t i2s_mic_enable(void);

/**
 * @brief Start mic streaming to pipe (event-driven mode)
 *
 * Starts a background task that reads from I2S and writes to pipe.
 * Sends SIG_MIC_DATA_READY to recorder entity when data is available.
 *
 * @param recorder_entity_id  Entity ID to receive SIG_MIC_DATA_READY
 * @return ESP_OK on success
 */
esp_err_t i2s_mic_start_stream(uint16_t recorder_entity_id);

/**
 * @brief Stop mic streaming
 */
esp_err_t i2s_mic_stop_stream(void);

/**
 * @brief Get mic data pipe for reading
 * @return Pointer to ur_pipe_t (cast from void* to avoid header dependency)
 */
void *i2s_mic_get_pipe(void);
