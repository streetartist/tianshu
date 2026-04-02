#ifndef I2S_SPEAKER_H
#define I2S_SPEAKER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

esp_err_t i2s_speaker_init(void);
esp_err_t i2s_speaker_start(void);
esp_err_t i2s_speaker_play(const uint8_t *data, size_t len);
esp_err_t i2s_speaker_stop(void);

#endif
