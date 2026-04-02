#ifndef WAKE_WORD_H
#define WAKE_WORD_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wake_word_init(void);
bool wake_word_detect(const int16_t *audio, int len);
int wake_word_get_chunksize(void);

#ifdef __cplusplus
}
#endif

#endif // WAKE_WORD_H
