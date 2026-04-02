/**
 * @file ent_recorder.c
 * @brief Audio recording entity implementation (event-driven)
 *
 * Records audio with VAD detection using pipe-based streaming.
 * Uses UR_AWAIT_SIGNAL to wait for mic data - pure event-driven.
 */

#include "ent_recorder.h"
#include "ur_core.h"
#include "ur_flow.h"
#include "ur_pipe.h"
#include "app_signals.h"
#include "app_config.h"
#include "app_buffers.h"

#include "i2s_mic.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ENT_REC";

/* ============================================================================
 * Static Buffers and Scratchpad
 * ========================================================================== */

/* Scratchpad for uFlow - only small state variables */
typedef struct {
    size_t total_samples;             /**< Total samples recorded */
    int silence_count;                /**< Consecutive silence frames */
    int voice_frames;                 /**< Frames with voice detected */
} recorder_scratch_t;

UR_SCRATCH_STATIC_ASSERT(recorder_scratch_t);

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

/**
 * @brief Calculate audio frame energy (average absolute value)
 */
static int32_t calc_audio_energy(const int16_t *samples, size_t count)
{
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += abs(samples[i]);
    }
    return (int32_t)(sum / count);
}

/* ============================================================================
 * State Machine
 * ========================================================================== */

static ur_entity_t s_recorder_entity;

/* Forward declarations */
static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_recording_entry(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_recording_exit(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t record_flow(ur_entity_t *ent, const ur_signal_t *sig);
static uint16_t on_record_cancel(ur_entity_t *ent, const ur_signal_t *sig);

/* State: IDLE */
static const ur_rule_t s_idle_rules[] = {
    UR_RULE(SIG_RECORD_START, STATE_REC_RECORDING, NULL),
    UR_RULE_END
};

/* State: RECORDING - event-driven by SIG_MIC_DATA_READY */
static const ur_rule_t s_recording_rules[] = {
    UR_RULE(SIG_RECORD_CANCEL, STATE_REC_IDLE, on_record_cancel),
    UR_RULE(SIG_MIC_DATA_READY, 0, record_flow),  /* Event-driven by mic */
    UR_RULE_END
};

/* State definitions */
static const ur_state_def_t s_states[] = {
    UR_STATE(STATE_REC_IDLE, 0, on_idle_entry, NULL, s_idle_rules),
    UR_STATE(STATE_REC_RECORDING, 0, on_recording_entry, on_recording_exit, s_recording_rules),
};

/* ============================================================================
 * Action Handlers
 * ========================================================================== */

static uint16_t on_idle_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Recorder idle");
    UR_FLOW_RESET(ent);
    return 0;
}

static uint16_t on_recording_entry(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Recording started");

    /* Initialize scratchpad */
    recorder_scratch_t *s = UR_SCRATCH_PTR(ent, recorder_scratch_t);
    memset(s, 0, sizeof(recorder_scratch_t));

    /* Reset audio buffer */
    audio_data_t *audio = app_get_audio_data();
    audio->sample_count = 0;

    /* Start mic streaming - it will send SIG_MIC_DATA_READY signals */
    i2s_mic_start_stream(ENT_ID_RECORDER);

    return 0;
}

static uint16_t on_recording_exit(ur_entity_t *ent, const ur_signal_t *sig)
{
    /* Stop mic streaming */
    i2s_mic_stop_stream();
    return 0;
}

static uint16_t on_record_cancel(ur_entity_t *ent, const ur_signal_t *sig)
{
    ESP_LOGI(TAG, "Recording cancelled");
    return 0;
}

/**
 * @brief Recording coroutine (event-driven)
 *
 * Waits for SIG_MIC_DATA_READY, reads from pipe, processes VAD.
 * Pure event-driven - no polling, no timeout.
 */
static uint16_t record_flow(ur_entity_t *ent, const ur_signal_t *sig)
{
    recorder_scratch_t *s = UR_SCRATCH_PTR(ent, recorder_scratch_t);
    audio_data_t *audio = app_get_audio_data();
    int16_t *buffer = app_get_audio_buffer();
    ur_pipe_t *mic_pipe = (ur_pipe_t *)i2s_mic_get_pipe();

    /* Frame buffer for reading from pipe */
    static int16_t frame_buf[VAD_FRAME_SIZE];

    UR_FLOW_BEGIN(ent);

    while (s->total_samples < AUDIO_BUFFER_SAMPLES) {
        /* Wait for mic data signal (event-driven) */
        UR_AWAIT_SIGNAL(ent, SIG_MIC_DATA_READY);

        /* Read all available frames from pipe */
        while (ur_pipe_available(mic_pipe) >= VAD_FRAME_SIZE * sizeof(int16_t)) {
            size_t read_bytes = ur_pipe_read(mic_pipe, frame_buf,
                                              VAD_FRAME_SIZE * sizeof(int16_t), 0);
            size_t read_samples = read_bytes / sizeof(int16_t);

            if (read_samples == 0) break;

            /* Check buffer overflow */
            if (s->total_samples + read_samples > AUDIO_BUFFER_SAMPLES) {
                read_samples = AUDIO_BUFFER_SAMPLES - s->total_samples;
            }

            /* Copy to main buffer */
            memcpy(buffer + s->total_samples, frame_buf, read_samples * sizeof(int16_t));
            s->total_samples += read_samples;

            /* Calculate energy for VAD */
            int32_t energy = calc_audio_energy(frame_buf, read_samples);

            if (energy > VAD_THRESHOLD) {
                s->voice_frames++;
                s->silence_count = 0;
            } else {
                s->silence_count++;
            }

            /* Check for end of speech */
            if (s->voice_frames >= VAD_MIN_FRAMES &&
                s->silence_count >= VAD_SILENCE_FRAMES) {
                ESP_LOGI(TAG, "Silence detected, stopping recording");
                goto recording_done;
            }
        }
    }

recording_done:
    /* Check if we got enough audio */
    if (s->total_samples < VAD_FRAME_SIZE * VAD_MIN_FRAMES) {
        ESP_LOGW(TAG, "Recording too short, ignoring");
        UR_FLOW_GOTO(ent, STATE_REC_IDLE);
    }

    /* Update audio data */
    audio->sample_count = s->total_samples;
    ESP_LOGI(TAG, "Recording done: %d samples", s->total_samples);

    /* Notify system */
    ur_emit_to_id(ENT_ID_SYSTEM,
        (ur_signal_t){ .id = SIG_RECORD_DONE, .src_id = ENT_ID_RECORDER,
                       .ptr = audio });

    /* Transition to idle */
    UR_FLOW_GOTO(ent, STATE_REC_IDLE);

    UR_FLOW_END(ent);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

ur_entity_t *ent_recorder_get(void)
{
    return &s_recorder_entity;
}

ur_err_t ent_recorder_init(void)
{
    /* Initialize entity */
    ur_entity_config_t cfg = {
        .id = ENT_ID_RECORDER,
        .name = "recorder",
        .states = s_states,
        .state_count = sizeof(s_states) / sizeof(s_states[0]),
        .initial_state = STATE_REC_IDLE,
        .user_data = NULL,
    };

    ur_err_t ret = ur_init(&s_recorder_entity, &cfg);
    if (ret != UR_OK) {
        ESP_LOGE(TAG, "Failed to init recorder entity: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Recorder entity initialized (event-driven mode)");
    return UR_OK;
}
