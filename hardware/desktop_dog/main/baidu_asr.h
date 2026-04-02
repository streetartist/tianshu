#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

esp_err_t baidu_asr_get_token(char *token, size_t token_size);
esp_err_t baidu_asr_recognize(const int16_t *audio, size_t samples, char *result, size_t result_size);
