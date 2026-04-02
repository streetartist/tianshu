#include "wake_word.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"

static const char *TAG = "WAKE_WORD";
static const char *k_wakenet_model = "wn9_hilexin";
static const char *k_wake_word_text = "Hi,Lexin";

static const esp_wn_iface_t *s_wakenet = NULL;
static model_iface_data_t *s_model_data = NULL;
static srmodel_list_t *s_models = NULL;

static int16_t *s_chunk_buf = NULL;
static size_t s_chunk_size = 0;
static size_t s_chunk_index = 0;

static void wake_word_release(void)
{
    if (s_model_data && s_wakenet) {
        s_wakenet->destroy(s_model_data);
    }
    s_model_data = NULL;
    s_wakenet = NULL;

    if (s_models) {
        esp_srmodel_deinit(s_models);
        s_models = NULL;
    }

    if (s_chunk_buf) {
        heap_caps_free(s_chunk_buf);
        s_chunk_buf = NULL;
    }

    s_chunk_size = 0;
    s_chunk_index = 0;
}

esp_err_t wake_word_init(void)
{
    if (s_model_data) {
        return ESP_OK;
    }

    s_models = esp_srmodel_init("model");
    if (!s_models) {
        ESP_LOGE(TAG, "Failed to load srmodels from partition 'model'");
        wake_word_release();
        return ESP_FAIL;
    }

    if (esp_srmodel_exists(s_models, (char *)k_wakenet_model) < 0) {
        ESP_LOGE(TAG, "WakeNet model not found: %s", k_wakenet_model);
        wake_word_release();
        return ESP_FAIL;
    }

    s_wakenet = esp_wn_handle_from_name(k_wakenet_model);
    if (!s_wakenet) {
        ESP_LOGE(TAG, "Failed to get WakeNet handle for model: %s", k_wakenet_model);
        wake_word_release();
        return ESP_FAIL;
    }

    s_model_data = s_wakenet->create(k_wakenet_model, DET_MODE_90);
    if (!s_model_data) {
        ESP_LOGE(TAG, "Failed to create WakeNet model: %s", k_wakenet_model);
        wake_word_release();
        return ESP_FAIL;
    }

    s_chunk_size = (size_t)s_wakenet->get_samp_chunksize(s_model_data);
    if (s_chunk_size == 0) {
        ESP_LOGE(TAG, "WakeNet chunk size is zero");
        wake_word_release();
        return ESP_FAIL;
    }

    s_chunk_buf = (int16_t *)heap_caps_malloc(s_chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_chunk_buf) {
        ESP_LOGE(TAG, "Failed to allocate WakeNet buffer (%u samples)", (unsigned)s_chunk_size);
        wake_word_release();
        return ESP_ERR_NO_MEM;
    }

    s_chunk_index = 0;

    int sample_rate = s_wakenet->get_samp_rate(s_model_data);
    ESP_LOGI(TAG, "WakeNet9 init: model=%s, wakeword=%s, chunk=%u, sr=%d",
             k_wakenet_model, k_wake_word_text, (unsigned)s_chunk_size, sample_rate);

    return ESP_OK;
}

bool wake_word_detect(const int16_t *audio, int len)
{
    if (!s_wakenet || !s_model_data || !s_chunk_buf || s_chunk_size == 0) {
        return false;
    }
    if (!audio || len <= 0) {
        return false;
    }

    int offset = 0;
    while (offset < len) {
        size_t copy = s_chunk_size - s_chunk_index;
        if (copy > (size_t)(len - offset)) {
            copy = (size_t)(len - offset);
        }

        memcpy(s_chunk_buf + s_chunk_index, audio + offset, copy * sizeof(int16_t));
        s_chunk_index += copy;
        offset += (int)copy;

        if (s_chunk_index >= s_chunk_size) {
            int res = s_wakenet->detect(s_model_data, s_chunk_buf);
            s_chunk_index = 0;
            if (res > 0) {
                ESP_LOGI(TAG, "Wake word detected: %s", k_wake_word_text);
                return true;
            }
        }
    }

    return false;
}

int wake_word_get_chunksize(void)
{
    return (int)s_chunk_size;
}
