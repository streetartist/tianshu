/**
 * Audio Player - MP3/OGG streaming playback for ESP32
 * Uses minimp3 for MP3 decoding, stb_vorbis for OGG decoding
 */

#include "audio_player.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

// SD card includes
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#include "minimp3.h"

// Rename stb_vorbis internal functions to avoid conflicts with minimp3
#define get_bits stb_vorbis_get_bits
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"
#undef get_bits

// Include audio headers for unified API
#include "i2s_mic.h"
#include "i2s_speaker.h"

static const char *TAG = "AUDIO_PLAYER";

// SD card configuration (SDMMC 4-bit mode)
#define SD_MOUNT_POINT "/sdcard"
#define SD_OGG_PATH    "/sdcard/ogg"
#define SD_CMD_PIN     11
#define SD_CLK_PIN     12
#define SD_D0_PIN      13
#define SD_D1_PIN      10
#define SD_D2_PIN      21
#define SD_D3_PIN      2

// SD card state
static sdmmc_card_t *s_sd_card = NULL;
static bool s_sd_mounted = false;

// Audio format types
typedef enum {
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_OGG,
    AUDIO_FORMAT_UNKNOWN
} audio_format_t;

// Detect audio format from URL (handles query params like .ogg?key=xxx)
static audio_format_t detect_audio_format(const char *url)
{
    if (!url) return AUDIO_FORMAT_UNKNOWN;

    const char *ext = strrchr(url, '.');
    if (!ext) return AUDIO_FORMAT_MP3;  // Default to MP3

    // Check for .ogg (with possible query params)
    if (strncasecmp(ext, ".ogg", 4) == 0) {
        return AUDIO_FORMAT_OGG;
    }
    // Check for .mp3
    if (strncasecmp(ext, ".mp3", 4) == 0) {
        return AUDIO_FORMAT_MP3;
    }

    return AUDIO_FORMAT_MP3;  // Default to MP3
}

// Player state
static volatile bool s_playing = false;
static volatile bool s_stop_requested = false;
static char s_play_url[512] = {0};
static TaskHandle_t s_play_task = NULL;
static StaticTask_t s_play_task_tcb;
static StackType_t *s_play_stack = NULL;

// Buffer sizes
#define MP3_BUF_SIZE    16384
#define PCM_BUF_SIZE    (MINIMP3_MAX_SAMPLES_PER_FRAME * 2)
#define OGG_READ_BUF_SIZE  8192
#define OGG_PCM_BUF_SIZE   4096
#define AUDIO_PLAY_TASK_STACK_BYTES (32 * 1024)
#define AUDIO_PLAY_TASK_STACK_WORDS (AUDIO_PLAY_TASK_STACK_BYTES / sizeof(StackType_t))

// OGG playback state
static char s_ogg_filename[128] = {0};
static bool s_play_ogg = false;

// Volume control (0-100)
static int s_volume = 80;

// Forward declarations
static void play_ogg_from_sd(void);
static void decode_and_play_ogg(uint8_t *ogg_buf, size_t ogg_size);

esp_err_t audio_player_init(void)
{
    ESP_LOGI(TAG, "Audio player initializing...");

    // Initialize SD card (SDMMC 4-bit mode)
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;  // High speed mode

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;  // 4-bit mode
    slot_config.clk = (gpio_num_t)SD_CLK_PIN;
    slot_config.cmd = (gpio_num_t)SD_CMD_PIN;
    slot_config.d0 = (gpio_num_t)SD_D0_PIN;
    slot_config.d1 = (gpio_num_t)SD_D1_PIN;
    slot_config.d2 = (gpio_num_t)SD_D2_PIN;
    slot_config.d3 = (gpio_num_t)SD_D3_PIN;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "SD pins: CLK=%d, CMD=%d, D0=%d, D1=%d, D2=%d, D3=%d",
             SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN, SD_D1_PIN, SD_D2_PIN, SD_D3_PIN);

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                            &mount_config, &s_sd_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card mount failed: %s (0x%x)", esp_err_to_name(ret), ret);
        s_sd_mounted = false;
    } else {
        s_sd_mounted = true;
        ESP_LOGI(TAG, "SD card mounted, size: %llu MB",
                 ((uint64_t)s_sd_card->csd.capacity) * s_sd_card->csd.sector_size / (1024 * 1024));

        // Create ogg directory if not exists
        struct stat st;
        if (stat(SD_OGG_PATH, &st) != 0) {
            int ret = mkdir(SD_OGG_PATH, 0755);
            if (ret == 0) {
                ESP_LOGI(TAG, "Created directory: %s", SD_OGG_PATH);
            } else {
                ESP_LOGE(TAG, "Failed to create directory: %s, errno=%d", SD_OGG_PATH, errno);
            }
        } else {
            ESP_LOGI(TAG, "Directory exists: %s", SD_OGG_PATH);
        }
    }

    ESP_LOGI(TAG, "Audio player initialized");
    return ESP_OK;
}

bool audio_player_is_playing(void)
{
    return s_playing;
}

esp_err_t audio_player_stop(void)
{
    s_stop_requested = true;
    return ESP_OK;
}

// Decode and play OGG data from memory buffer
static void decode_and_play_ogg(uint8_t *ogg_buf, size_t ogg_size)
{
    bool speaker_started = false;
    int16_t *pcm_buf = NULL;
    stb_vorbis *vorbis = NULL;

    // Open vorbis decoder
    int error = 0;
    vorbis = stb_vorbis_open_memory(ogg_buf, ogg_size, &error, NULL);
    if (!vorbis) {
        ESP_LOGE(TAG, "stb_vorbis_open_memory failed: %d", error);
        goto decode_cleanup;
    }

    // Get info
    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    ESP_LOGI(TAG, "OGG: %d Hz, %d ch", info.sample_rate, info.channels);

    // Allocate PCM buffer
    pcm_buf = heap_caps_malloc(OGG_PCM_BUF_SIZE * info.channels * sizeof(int16_t),
                               MALLOC_CAP_SPIRAM);
    if (!pcm_buf) {
        ESP_LOGE(TAG, "Failed to allocate PCM buffer");
        goto decode_cleanup;
    }

    // Start speaker
    if (i2s_speaker_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start speaker");
        goto decode_cleanup;
    }
    speaker_started = true;

    // Decode and play loop
    int total_samples = 0;
    while (!s_stop_requested) {
        int samples = stb_vorbis_get_samples_short_interleaved(
            vorbis, info.channels, pcm_buf, OGG_PCM_BUF_SIZE * info.channels);

        if (samples == 0) break;

        // Convert stereo to mono if needed
        if (info.channels == 2) {
            for (int i = 0; i < samples; i++) {
                int32_t left = pcm_buf[i * 2];
                int32_t right = pcm_buf[i * 2 + 1];
                pcm_buf[i] = (int16_t)((left + right) / 2);
            }
        }

        // Apply volume
        for (int i = 0; i < samples; i++) {
            pcm_buf[i] = (int16_t)(pcm_buf[i] * s_volume / 100);
        }

        if (i2s_speaker_play((uint8_t *)pcm_buf, samples * sizeof(int16_t)) != ESP_OK) {
            ESP_LOGE(TAG, "Speaker write failed");
            break;
        }

        total_samples += samples;
        if (total_samples % 10000 == 0) {
            vTaskDelay(1);
        }
    }

    ESP_LOGI(TAG, "OGG played %d samples", total_samples);

decode_cleanup:
    if (speaker_started) i2s_speaker_stop();
    if (vorbis) stb_vorbis_close(vorbis);
    if (pcm_buf) heap_caps_free(pcm_buf);
}

// Play OGG from URL (download to SD card first)
static void play_ogg_from_url(void)
{
    ESP_LOGI(TAG, "Playing OGG from URL: %s", s_play_url);

    if (!s_sd_mounted) {
        ESP_LOGE(TAG, "SD card not mounted, cannot download OGG");
        return;
    }

    esp_http_client_handle_t client = NULL;
    FILE *fp = NULL;
    uint8_t *buf = NULL;
    bool client_opened = false;
    char filepath[128];

    // Use fixed temp filename for streaming music
    snprintf(filepath, sizeof(filepath), "%s/temp_music.ogg", SD_OGG_PATH);

    // HTTP client config
    esp_http_client_config_t config = {
        .url = s_play_url,
        .timeout_ms = 30000,
        .buffer_size = 8192,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client init failed");
        goto url_cleanup;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        goto url_cleanup;
    }
    client_opened = true;

    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "OGG content length: %d", content_length);

    // Open file for writing
    fp = fopen(filepath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to create file: %s, errno=%d", filepath, errno);
        goto url_cleanup;
    }

    // Allocate download buffer
    buf = malloc(8192);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate download buffer");
        goto url_cleanup;
    }

    // Download to SD card
    int total_read = 0;
    while (!s_stop_requested) {
        int read = esp_http_client_read(client, (char *)buf, 8192);
        if (read > 0) {
            fwrite(buf, 1, read, fp);
            total_read += read;
        } else if (read == 0) {
            break;
        } else {
            ESP_LOGE(TAG, "HTTP read error");
            goto url_cleanup;
        }
    }

    fclose(fp);
    fp = NULL;
    ESP_LOGI(TAG, "Downloaded %d bytes to SD card", total_read);

    // Close HTTP
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    client = NULL;
    client_opened = false;

    // Play from SD card
    if (!s_stop_requested) {
        strncpy(s_ogg_filename, "temp_music.ogg", sizeof(s_ogg_filename) - 1);
        play_ogg_from_sd();
    }

url_cleanup:
    if (fp) fclose(fp);
    if (buf) free(buf);
    if (client) {
        if (client_opened) esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }
}

// Play OGG file from SD card
static void play_ogg_from_sd(void)
{
    ESP_LOGI(TAG, "Playing OGG: %s", s_ogg_filename);

    FILE *fp = NULL;
    uint8_t *ogg_buf = NULL;

    // Build full path
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", SD_OGG_PATH, s_ogg_filename);

    // Open file
    fp = fopen(filepath, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open: %s", filepath);
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    ESP_LOGI(TAG, "OGG file size: %ld bytes", file_size);

    // Allocate buffer for entire file (in PSRAM)
    ogg_buf = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (!ogg_buf) {
        ESP_LOGE(TAG, "Failed to allocate OGG buffer");
        fclose(fp);
        return;
    }

    // Read entire file
    size_t read_size = fread(ogg_buf, 1, file_size, fp);
    fclose(fp);

    if (read_size != file_size) {
        ESP_LOGE(TAG, "Failed to read file");
        heap_caps_free(ogg_buf);
        return;
    }

    // Decode and play
    decode_and_play_ogg(ogg_buf, file_size);
    heap_caps_free(ogg_buf);
}

// Play task - runs in separate task with larger stack
static void play_task(void *arg)
{
    ESP_LOGI(TAG, "Play task started");

    // Check if playing OGG from SD card
    if (s_play_ogg) {
        play_ogg_from_sd();
        s_playing = false;
        s_play_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    // Check URL format
    audio_format_t format = detect_audio_format(s_play_url);
    if (format == AUDIO_FORMAT_OGG) {
        play_ogg_from_url();
        s_playing = false;
        s_play_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    // MP3 playback from URL
    bool speaker_started = false;
    bool client_opened = false;
    esp_http_client_handle_t client = NULL;
    uint8_t *mp3_buf = NULL;
    int16_t *pcm_buf = NULL;
    mp3dec_t *mp3d = NULL;

    // 注意：不禁用麦克风，TX和RX共享时钟

    // Allocate all buffers in PSRAM
    mp3_buf = heap_caps_malloc(MP3_BUF_SIZE, MALLOC_CAP_SPIRAM);
    pcm_buf = heap_caps_malloc(PCM_BUF_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    mp3d = heap_caps_malloc(sizeof(mp3dec_t), MALLOC_CAP_SPIRAM);

    if (!mp3_buf || !pcm_buf || !mp3d) {
        ESP_LOGE(TAG, "Failed to allocate buffers");
        goto cleanup;
    }

    mp3dec_init(mp3d);

    // HTTP client config
    esp_http_client_config_t config = {
        .url = s_play_url,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client init failed");
        goto cleanup;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    client_opened = true;
    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "Content length: %d", content_length);

    // Streaming decode loop
    size_t mp3_len = 0;
    size_t mp3_pos = 0;
    int total_samples = 0;

    while (!s_stop_requested) {
        if (mp3_len - mp3_pos < 4096) {
            if (mp3_pos > 0) {
                memmove(mp3_buf, mp3_buf + mp3_pos, mp3_len - mp3_pos);
                mp3_len -= mp3_pos;
                mp3_pos = 0;
            }

            int read = esp_http_client_read(client, (char *)(mp3_buf + mp3_len),
                                            MP3_BUF_SIZE - mp3_len);
            if (read > 0) {
                mp3_len += read;
            } else if (read == 0) {
                if (mp3_len == mp3_pos) break;
            } else {
                ESP_LOGE(TAG, "HTTP read error");
                break;
            }
        }

        mp3dec_frame_info_t info;
        int samples = mp3dec_decode_frame(mp3d, mp3_buf + mp3_pos,
                                          mp3_len - mp3_pos, pcm_buf, &info);

        if (samples > 0) {
            // Start speaker on first decode
            if (!speaker_started && info.hz > 0) {
                ESP_LOGI(TAG, "MP3: %d Hz, %d ch, %d kbps",
                         info.hz, info.channels, info.bitrate_kbps);
                if (i2s_speaker_start() != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to start speaker");
                    break;
                }
                speaker_started = true;
            }

            // Convert stereo to mono if needed
            if (info.channels == 2) {
                // 立体声混音为单声道: (L + R) / 2
                for (int i = 0; i < samples; i++) {
                    int32_t left = pcm_buf[i * 2];
                    int32_t right = pcm_buf[i * 2 + 1];
                    pcm_buf[i] = (int16_t)((left + right) / 2);
                }
            }

            // Apply volume
            for (int i = 0; i < samples; i++) {
                pcm_buf[i] = (int16_t)(pcm_buf[i] * s_volume / 100);
            }

            if (i2s_speaker_play((uint8_t *)pcm_buf, samples * sizeof(int16_t)) != ESP_OK) {
                ESP_LOGE(TAG, "Speaker write failed");
                break;
            }
            total_samples += samples;
        }

        if (info.frame_bytes > 0) {
            mp3_pos += info.frame_bytes;
        } else {
            mp3_pos++;
        }

        // Yield to other tasks occasionally (i2s_channel_write already blocks)
        if (total_samples % 10000 == 0) {
            vTaskDelay(1);
        }
    }

    ESP_LOGI(TAG, "Played %d samples", total_samples);

cleanup:
    if (speaker_started) {
        i2s_speaker_stop();
    }
    if (client) {
        if (client_opened) {
            esp_http_client_close(client);
        }
        esp_http_client_cleanup(client);
    }
    if (mp3_buf) heap_caps_free(mp3_buf);
    if (pcm_buf) heap_caps_free(pcm_buf);
    if (mp3d) heap_caps_free(mp3d);
    s_playing = false;
    s_play_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t audio_player_play_url(const char *url)
{
    if (!url) return ESP_ERR_INVALID_ARG;
    if (s_playing) {
        ESP_LOGW(TAG, "Already playing");
        return ESP_ERR_INVALID_STATE;
    }

    // Save URL
    strncpy(s_play_url, url, sizeof(s_play_url) - 1);
    s_play_url[sizeof(s_play_url) - 1] = '\0';

    s_playing = true;
    s_stop_requested = false;
    s_play_ogg = false;  // Playing MP3 from URL

    // Create play task with a larger stack (minimp3 + TLS can exceed 16 KB).
    if (!s_play_stack) {
        s_play_stack = heap_caps_malloc(AUDIO_PLAY_TASK_STACK_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_play_stack) {
            ESP_LOGE(TAG, "Failed to allocate play task stack");
            s_playing = false;
            return ESP_ERR_NO_MEM;
        }
    }

    s_play_task = xTaskCreateStaticPinnedToCore(play_task, "audio_play",
                                                AUDIO_PLAY_TASK_STACK_WORDS, NULL, 5,
                                                s_play_stack, &s_play_task_tcb, tskNO_AFFINITY);
    if (!s_play_task) {
        ESP_LOGE(TAG, "Failed to create play task");
        s_playing = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Play task created for: %s", url);
    return ESP_OK;
}

esp_err_t audio_player_play_ogg(const char *filename)
{
    if (!filename) return ESP_ERR_INVALID_ARG;
    if (s_playing) {
        ESP_LOGW(TAG, "Already playing");
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_sd_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    // Save filename
    strncpy(s_ogg_filename, filename, sizeof(s_ogg_filename) - 1);
    s_ogg_filename[sizeof(s_ogg_filename) - 1] = '\0';

    s_playing = true;
    s_stop_requested = false;
    s_play_ogg = true;

    // Create play task
    if (!s_play_stack) {
        s_play_stack = heap_caps_malloc(AUDIO_PLAY_TASK_STACK_BYTES,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_play_stack) {
            ESP_LOGE(TAG, "Failed to allocate play task stack");
            s_playing = false;
            s_play_ogg = false;
            return ESP_ERR_NO_MEM;
        }
    }

    s_play_task = xTaskCreateStaticPinnedToCore(play_task, "audio_play",
                                                AUDIO_PLAY_TASK_STACK_WORDS, NULL, 5,
                                                s_play_stack, &s_play_task_tcb, tskNO_AFFINITY);
    if (!s_play_task) {
        ESP_LOGE(TAG, "Failed to create play task");
        s_playing = false;
        s_play_ogg = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Play task created for OGG: %s", filename);
    return ESP_OK;
}

bool audio_player_sd_mounted(void)
{
    return s_sd_mounted;
}

void audio_player_set_volume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    s_volume = volume;
    ESP_LOGI(TAG, "Volume set to %d", s_volume);
}

int audio_player_get_volume(void)
{
    return s_volume;
}
