/**
 * @file pet_attributes.c
 * @brief Pet attribute management and decay logic
 */

#include "pet_attributes.h"
#include "esp_log.h"

static const char *TAG = "PET_ATTR";

/* Clamp value to 0-100 range */
static inline uint8_t clamp_100(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (uint8_t)value;
}

void pet_attributes_init(pet_data_t *data)
{
    if (!data) return;

    /* Set default values if not loaded from NVS */
    if (data->stats.happiness == 0 &&
        data->stats.hunger == 0 &&
        data->stats.fatigue == 0) {
        data->stats.happiness = 50;
        data->stats.hunger = 50;
        data->stats.fatigue = 50;
    }

    data->current_mood = pet_attributes_calculate_mood(data);
    ESP_LOGI(TAG, "Pet attributes initialized: H=%d, Hu=%d, F=%d, Mood=%d",
             data->stats.happiness, data->stats.hunger,
             data->stats.fatigue, data->current_mood);
}

bool pet_attributes_decay(pet_data_t *data)
{
    if (!data) return false;

    /* Don't decay if dead */
    if (pet_attributes_is_dead(data)) {
        return false;
    }

    uint8_t old_happiness = data->stats.happiness;
    uint8_t old_hunger = data->stats.hunger;
    uint8_t old_fatigue = data->stats.fatigue;

    /* Apply decay:
     * - happiness: -1
     * - hunger: +1 (more hungry)
     * - fatigue: +1 (more tired)
     */
    data->stats.happiness = clamp_100(data->stats.happiness - 1);
    data->stats.hunger = clamp_100(data->stats.hunger + 1);
    data->stats.fatigue = clamp_100(data->stats.fatigue + 1);

    bool changed = (data->stats.happiness != old_happiness ||
                    data->stats.hunger != old_hunger ||
                    data->stats.fatigue != old_fatigue);

    if (changed) {
        ESP_LOGD(TAG, "Decay: H=%d->%d, Hu=%d->%d, F=%d->%d",
                 old_happiness, data->stats.happiness,
                 old_hunger, data->stats.hunger,
                 old_fatigue, data->stats.fatigue);
    }

    return changed;
}

void pet_attributes_feed(pet_data_t *data, bool force)
{
    if (!data || pet_attributes_is_dead(data)) return;

    /* Feed: hunger-30, happiness+5 */
    data->stats.hunger = clamp_100(data->stats.hunger - 30);
    data->stats.happiness = clamp_100(data->stats.happiness + 5);

    ESP_LOGI(TAG, "Pet fed: Hunger=%d, Happiness=%d",
             data->stats.hunger, data->stats.happiness);
}

void pet_attributes_play(pet_data_t *data)
{
    if (!data || pet_attributes_is_dead(data)) return;

    /* Play: happiness+20, fatigue+10 */
    data->stats.happiness = clamp_100(data->stats.happiness + 20);
    data->stats.fatigue = clamp_100(data->stats.fatigue + 10);

    ESP_LOGI(TAG, "Pet played: Happiness=%d, Fatigue=%d",
             data->stats.happiness, data->stats.fatigue);
}

void pet_attributes_sleep(pet_data_t *data, bool force)
{
    if (!data || pet_attributes_is_dead(data)) return;

    /* Sleep: fatigue-40 */
    data->stats.fatigue = clamp_100(data->stats.fatigue - 40);

    ESP_LOGI(TAG, "Pet rested: Fatigue=%d", data->stats.fatigue);
}

void pet_attributes_revive(pet_data_t *data)
{
    if (!data) return;

    /* Revive: reset all to 50 */
    data->stats.happiness = 50;
    data->stats.hunger = 50;
    data->stats.fatigue = 50;
    data->current_mood = PET_MOOD_NORMAL;

    ESP_LOGI(TAG, "Pet revived!");
}

pet_mood_t pet_attributes_calculate_mood(const pet_data_t *data)
{
    if (!data) return PET_MOOD_NORMAL;

    /* Check death conditions */
    if (data->stats.happiness == 0 ||
        data->stats.hunger >= 100 ||
        data->stats.fatigue >= 100) {
        return PET_MOOD_DEAD;
    }

    /* Count abnormal conditions */
    int abnormal = 0;
    if (data->stats.happiness < 30) abnormal++;
    if (data->stats.hunger > 70) abnormal++;
    if (data->stats.fatigue > 70) abnormal++;

    /* Sick if multiple conditions are abnormal */
    if (abnormal >= 2) {
        return PET_MOOD_SICK;
    }

    /* Individual conditions */
    if (data->stats.happiness > 70) {
        return PET_MOOD_HAPPY;
    }
    if (data->stats.happiness < 30) {
        return PET_MOOD_SAD;
    }
    if (data->stats.hunger > 70) {
        return PET_MOOD_HUNGRY;
    }
    if (data->stats.fatigue > 70) {
        return PET_MOOD_TIRED;
    }

    return PET_MOOD_NORMAL;
}

bool pet_attributes_is_dead(const pet_data_t *data)
{
    if (!data) return false;

    return (data->stats.happiness == 0 ||
            data->stats.hunger >= 100 ||
            data->stats.fatigue >= 100);
}

const char *pet_attributes_mood_name(pet_mood_t mood)
{
    switch (mood) {
        case PET_MOOD_HAPPY:  return "happy";
        case PET_MOOD_NORMAL: return "normal";
        case PET_MOOD_SAD:    return "sad";
        case PET_MOOD_HUNGRY: return "hungry";
        case PET_MOOD_TIRED:  return "tired";
        case PET_MOOD_SICK:   return "sick";
        case PET_MOOD_DEAD:   return "dead";
        default:              return "unknown";
    }
}
