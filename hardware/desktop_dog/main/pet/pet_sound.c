/**
 * @file pet_sound.c
 * @brief Pet sound feedback using TTS
 */

#include "pet_sound.h"
#include "baidu_tts.h"
#include "esp_log.h"

static const char *TAG = "PET_SND";

/* Sound file definitions */
typedef struct {
    pet_sound_t type;
    const char *filename;
    const char *text;
} pet_sound_def_t;

static const pet_sound_def_t s_sounds[] = {
    { PET_SOUND_FEED,    "pet_feed.pcm",    "好吃！" },
    { PET_SOUND_PLAY,    "pet_play.pcm",    "真开心！" },
    { PET_SOUND_SLEEP,   "pet_sleep.pcm",   "晚安~" },
    { PET_SOUND_HUNGRY,  "pet_hungry.pcm",  "我饿了..." },
    { PET_SOUND_TIRED,   "pet_tired.pcm",   "好困..." },
    { PET_SOUND_DEAD,    "pet_dead.pcm",    "我不行了..." },
    { PET_SOUND_REVIVE,  "pet_revive.pcm",  "我又活过来了！" },
};

#define NUM_SOUNDS (sizeof(s_sounds) / sizeof(s_sounds[0]))

esp_err_t pet_sound_init(void)
{
    ESP_LOGI(TAG, "Generating pet sound prompts...");

    /* Generate all pet sounds if they don't exist */
    for (int i = 0; i < NUM_SOUNDS; i++) {
        esp_err_t err = baidu_tts_generate_prompt(s_sounds[i].text, s_sounds[i].filename);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to generate %s", s_sounds[i].filename);
        }
    }

    ESP_LOGI(TAG, "Pet sound module initialized");
    return ESP_OK;
}

esp_err_t pet_sound_play(pet_sound_t sound)
{
    /* Find sound definition */
    const pet_sound_def_t *def = NULL;
    for (int i = 0; i < NUM_SOUNDS; i++) {
        if (s_sounds[i].type == sound) {
            def = &s_sounds[i];
            break;
        }
    }

    if (!def) {
        ESP_LOGW(TAG, "Unknown sound type: %d", sound);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Playing: %s", def->filename);
    return baidu_tts_play_prompt(def->filename);
}

bool pet_sound_is_playing(void)
{
    return baidu_tts_is_playing();
}
