#include "baidu_tts.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "i2s_speaker.h"
#include "i2s_mic.h"
#include "wake_word.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "ur_core.h"
#include "app_signals.h"
#include "app_config.h"

static const char *TAG = "BAIDU_TTS";

// Async TTS state
static volatile bool s_tts_playing = false;
static volatile bool s_tts_stop_requested = false;
static TaskHandle_t s_tts_task = NULL;
static char *s_tts_text = NULL;  // 动态分配
static uint16_t s_notify_entity_id = 0;  // Entity to notify when done

// Pipeline: audio segment queue
#define SEGMENT_QUEUE_SIZE 2
#define MAX_SEGMENT_AUDIO_SIZE (64 * 1024)  // 64KB per segment
typedef struct {
    uint8_t *data;
    size_t len;
} audio_segment_t;

#define TTS_SERVER "tsn.baidu.com"
#define TTS_PORT   "80"
#define CUID       "esp32s3_desktop_dog"
#define TTS_SAMPLE_RATE    16000
#define OUTPUT_SAMPLE_RATE 44100
#define PROMPT_PATH "/sdcard/ogg"
#define MAX_SEGMENT_CHARS  60  // 每段最大字符数

extern char s_access_token[512];

static void url_encode(const char *src, char *dst, size_t dst_size)
{
    const char *hex = "0123456789ABCDEF";
    size_t pos = 0;
    while (*src && pos < dst_size - 4) {
        unsigned char c = (unsigned char)*src;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[pos++] = c;
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex[c >> 4];
            dst[pos++] = hex[c & 0x0F];
        }
        src++;
    }
    dst[pos] = '\0';
}

// Check if byte is start of Chinese punctuation (。！？)
static bool is_chinese_punct(const char *p)
{
    unsigned char c0 = (unsigned char)p[0];
    unsigned char c1 = (unsigned char)p[1];
    unsigned char c2 = (unsigned char)p[2];
    // 。= E3 80 82
    if (c0 == 0xE3 && c1 == 0x80 && c2 == 0x82) return true;
    // ！= EF BC 81
    if (c0 == 0xEF && c1 == 0xBC && c2 == 0x81) return true;
    // ？= EF BC 9F
    if (c0 == 0xEF && c1 == 0xBC && c2 == 0x9F) return true;
    return false;
}

// Split text into segments at sentence boundaries
// Returns array of segment strings, caller must free each and the array
static char **split_text_segments(const char *text, int *out_count)
{
    if (!text || !out_count) return NULL;
    *out_count = 0;

    size_t text_len = strlen(text);
    if (text_len == 0) return NULL;

    // Estimate max segments
    int max_segs = (text_len / 30) + 2;
    char **segments = malloc(max_segs * sizeof(char *));
    if (!segments) return NULL;

    const char *p = text;
    const char *seg_start = text;
    const char *last_punct = NULL;  // Last sentence-ending punctuation
    int last_punct_skip = 0;
    int seg_count = 0;
    int char_count = 0;

    while (*p && seg_count < max_segs - 1) {
        // Count characters (UTF-8 aware)
        if ((*p & 0xC0) != 0x80) {
            char_count++;
        }

        // Check for sentence end punctuation, remember position
        if (is_chinese_punct(p)) {
            last_punct = p;
            last_punct_skip = 3;
        } else if (*p == '.' || *p == '!' || *p == '?') {
            last_punct = p;
            last_punct_skip = 1;
        }

        // When exceeding 60 chars, split at last punctuation
        if (char_count >= MAX_SEGMENT_CHARS) {
            size_t seg_len;
            const char *next_start;

            if (last_punct && last_punct > seg_start) {
                // Split at last punctuation
                seg_len = (last_punct - seg_start) + last_punct_skip;
                next_start = last_punct + last_punct_skip;
            } else {
                // No punctuation, force split here
                seg_len = p - seg_start;
                next_start = p;
            }

            char *seg = malloc(seg_len + 1);
            if (seg) {
                memcpy(seg, seg_start, seg_len);
                seg[seg_len] = '\0';
                segments[seg_count++] = seg;
            }

            seg_start = next_start;
            p = next_start;
            last_punct = NULL;
            char_count = 0;
            continue;
        }

        p++;
    }

    // Last segment
    if (seg_start && *seg_start) {
        size_t seg_len = strlen(seg_start);
        char *seg = malloc(seg_len + 1);
        if (seg) {
            strcpy(seg, seg_start);
            segments[seg_count++] = seg;
        }
    }

    *out_count = seg_count;
    return segments;
}

// 16kHz -> 44.1kHz resample (ratio 2.75625)
static size_t resample_to_44k(const int16_t *in, size_t in_samples,
                               int16_t *out, size_t out_max)
{
    // 44100 / 16000 = 2.75625
    const float ratio = (float)OUTPUT_SAMPLE_RATE / TTS_SAMPLE_RATE;
    size_t out_samples = (size_t)(in_samples * ratio);
    if (out_samples > out_max) out_samples = out_max;

    for (size_t i = 0; i < out_samples; i++) {
        float src_pos = i / ratio;
        size_t idx = (size_t)src_pos;
        float frac = src_pos - idx;

        if (idx + 1 < in_samples) {
            // Linear interpolation
            out[i] = (int16_t)(in[idx] * (1.0f - frac) + in[idx + 1] * frac);
        } else if (idx < in_samples) {
            out[i] = in[idx];
        } else {
            out[i] = 0;
        }
    }
    return out_samples;
}

// Synthesize one segment, return audio data (caller must free)
// Returns resampled 44.1kHz PCM data
static esp_err_t synth_segment(const char *text, uint8_t **out_data, size_t *out_len)
{
    if (!text || !out_data || !out_len) return ESP_ERR_INVALID_ARG;
    *out_data = NULL;
    *out_len = 0;

    size_t text_len = strlen(text);
    if (text_len == 0) return ESP_ERR_INVALID_ARG;

    // Remove markdown
    char *clean = malloc(text_len + 1);
    if (!clean) return ESP_ERR_NO_MEM;

    size_t j = 0;
    for (size_t i = 0; i < text_len; i++) {
        if (text[i] == '*' || text[i] == '#' || text[i] == '`') continue;
        clean[j++] = text[i];
    }
    clean[j] = '\0';

    // URL encode (double)
    size_t enc1_size = j * 3 + 1;
    size_t enc2_size = enc1_size * 3 + 1;
    char *enc1 = malloc(enc1_size);
    char *enc2 = malloc(enc2_size);
    if (!enc1 || !enc2) {
        free(clean);
        if (enc1) free(enc1);
        if (enc2) free(enc2);
        return ESP_ERR_NO_MEM;
    }
    url_encode(clean, enc1, enc1_size);
    url_encode(enc1, enc2, enc2_size);
    free(clean);
    free(enc1);

    // Connect to server
    const struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res;
    if (getaddrinfo(TTS_SERVER, TTS_PORT, &hints, &res) != 0 || !res) {
        free(enc2);
        return ESP_FAIL;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        freeaddrinfo(res);
        free(enc2);
        return ESP_FAIL;
    }

    struct timeval tv = { .tv_sec = 15 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(res);
        free(enc2);
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    // Send request
    char post_data[2048];
    int post_len = snprintf(post_data, sizeof(post_data),
        "tex=%s&lan=zh&cuid=%s&ctp=1&aue=4&spd=5&pit=5&vol=9&per=0&tok=%s",
        enc2, CUID, s_access_token);
    free(enc2);

    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "POST /text2audio HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n\r\n",
        TTS_SERVER, post_len);

    write(sock, header, header_len);
    write(sock, post_data, post_len);

    // Read headers
    char recv_buf[512] = {0};
    int recv_len = 0;
    while (recv_len < (int)sizeof(recv_buf) - 1) {
        int n = read(sock, recv_buf + recv_len, 1);
        if (n <= 0) break;
        recv_len++;
        if (recv_len >= 4 && strncmp(recv_buf + recv_len - 4, "\r\n\r\n", 4) == 0)
            break;
    }

    if (!strstr(recv_buf, "audio")) {
        ESP_LOGE(TAG, "Synth error: %s", recv_buf);
        close(sock);
        return ESP_FAIL;
    }

    int content_length = -1;
    char *cl = strstr(recv_buf, "Content-Length:");
    if (cl) content_length = atoi(cl + 15);

    // Read all audio data
    size_t buf_cap = content_length > 0 ? content_length : 32768;
    uint8_t *raw_buf = heap_caps_malloc(buf_cap, MALLOC_CAP_SPIRAM);
    if (!raw_buf) {
        close(sock);
        return ESP_ERR_NO_MEM;
    }

    size_t total = 0;
    while (total < buf_cap) {
        int n = read(sock, raw_buf + total, buf_cap - total);
        if (n <= 0) break;
        total += n;
        if (content_length > 0 && (int)total >= content_length) break;
    }
    close(sock);

    if (total == 0) {
        heap_caps_free(raw_buf);
        return ESP_FAIL;
    }

    // Resample 16kHz -> 44.1kHz
    size_t in_samples = total / 2;
    size_t out_samples = (size_t)(in_samples * 2.76f) + 100;
    int16_t *resampled = heap_caps_malloc(out_samples * 2, MALLOC_CAP_SPIRAM);
    if (!resampled) {
        heap_caps_free(raw_buf);
        return ESP_ERR_NO_MEM;
    }

    out_samples = resample_to_44k((int16_t *)raw_buf, in_samples, resampled, out_samples);
    heap_caps_free(raw_buf);

    *out_data = (uint8_t *)resampled;
    *out_len = out_samples * 2;
    return ESP_OK;
}

// Internal TTS implementation with pipeline (blocking)
static esp_err_t tts_speak_internal(const char *text)
{
    ESP_LOGI(TAG, "TTS: %s", text);

    // Split text into segments
    int seg_count = 0;
    char **segments = split_text_segments(text, &seg_count);
    if (!segments || seg_count == 0) {
        ESP_LOGW(TAG, "No segments to speak");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Split into %d segments", seg_count);

    esp_err_t ret = ESP_OK;
    bool speaker_started = false;

    // Start speaker once
    if (i2s_speaker_start() != ESP_OK) {
        ESP_LOGE(TAG, "Speaker start failed");
        ret = ESP_FAIL;
        goto cleanup;
    }
    speaker_started = true;

    // Pipeline: synth next while playing current
    uint8_t *next_audio = NULL;
    size_t next_len = 0;

    for (int i = 0; i < seg_count && !s_tts_stop_requested; i++) {
        ESP_LOGI(TAG, "Segment %d: %s", i, segments[i]);

        // Start synthesizing current segment (or use pre-fetched)
        uint8_t *cur_audio = next_audio;
        size_t cur_len = next_len;
        next_audio = NULL;
        next_len = 0;

        if (!cur_audio) {
            // First segment, synthesize now
            if (synth_segment(segments[i], &cur_audio, &cur_len) != ESP_OK) {
                ESP_LOGE(TAG, "Synth failed for segment %d", i);
                continue;
            }
        }

        // Start pre-fetching next segment in background (simple: just synth before play)
        // For true pipeline, would need separate task, but this is simpler
        if (i + 1 < seg_count && !s_tts_stop_requested) {
            synth_segment(segments[i + 1], &next_audio, &next_len);
        }

        // Play current segment
        if (cur_audio && cur_len > 0) {
            // Play in chunks
            size_t offset = 0;
            while (offset < cur_len && !s_tts_stop_requested) {
                size_t chunk = cur_len - offset;
                if (chunk > 8192) chunk = 8192;
                i2s_speaker_play(cur_audio + offset, chunk);
                offset += chunk;
            }
            heap_caps_free(cur_audio);
        }
    }

cleanup:
    // Free any remaining pre-fetched audio
    if (next_audio) heap_caps_free(next_audio);

    if (speaker_started) i2s_speaker_stop();

    // Free segments
    for (int i = 0; i < seg_count; i++) {
        free(segments[i]);
    }
    free(segments);

    return ret;
}

// TTS task function
static void tts_task(void *arg)
{
    ESP_LOGI(TAG, "TTS task started");
    tts_speak_internal(s_tts_text);

    // Free text after use
    if (s_tts_text) {
        free(s_tts_text);
        s_tts_text = NULL;
    }

    s_tts_playing = false;
    s_tts_task = NULL;

    // Notify entity that playback is done (event-driven)
    if (s_notify_entity_id != 0) {
        ur_signal_t done_sig = { .id = SIG_TTS_PLAYBACK_DONE, .src_id = 0 };
        done_sig.payload.u32[0] = s_tts_stop_requested ? 1 : 0;  // 1 = was stopped
        ur_emit_to_id(s_notify_entity_id, done_sig);
    }

    vTaskDelete(NULL);
}

// Public API: Start async TTS playback (event-driven)
esp_err_t baidu_tts_speak_async(const char *text, uint16_t notify_entity_id)
{
    if (!text || strlen(text) == 0) return ESP_ERR_INVALID_ARG;

    if (s_tts_playing) {
        ESP_LOGW(TAG, "TTS already playing");
        return ESP_ERR_INVALID_STATE;
    }

    // Save text (动态分配)
    if (s_tts_text) {
        free(s_tts_text);
    }
    s_tts_text = strdup(text);
    if (!s_tts_text) {
        return ESP_ERR_NO_MEM;
    }

    s_tts_playing = true;
    s_tts_stop_requested = false;
    s_notify_entity_id = notify_entity_id;

    // Create TTS task
    BaseType_t ret = xTaskCreate(tts_task, "tts_play", 8192, NULL, 5, &s_tts_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TTS task");
        s_tts_playing = false;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

// Public API: Start async TTS playback (legacy, no notification)
esp_err_t baidu_tts_speak(const char *text)
{
    return baidu_tts_speak_async(text, 0);
}

// Stop TTS playback
esp_err_t baidu_tts_stop(void)
{
    s_tts_stop_requested = true;
    return ESP_OK;
}

// Check if TTS is playing
bool baidu_tts_is_playing(void)
{
    return s_tts_playing;
}

// Wait for TTS to finish (blocking)
void baidu_tts_wait(void)
{
    while (s_tts_playing) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Check if prompt file exists and has valid size
static bool prompt_exists(const char *filename)
{
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", PROMPT_PATH, filename);
    FILE *f = fopen(filepath, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        // 文件必须大于100字节才算有效
        if (size > 100) {
            return true;
        }
        ESP_LOGW(TAG, "Prompt file too small: %s (%ld bytes), will regenerate", filename, size);
    }
    return false;
}

// Generate prompt and save to SD card (blocking)
esp_err_t baidu_tts_generate_prompt(const char *text, const char *filename)
{
    if (prompt_exists(filename)) {
        ESP_LOGI(TAG, "Prompt already exists: %s", filename);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Generating prompt: %s -> %s", text, filename);

    char encoded1[256] = {0};
    char encoded2[512] = {0};
    url_encode(text, encoded1, sizeof(encoded1));
    url_encode(encoded1, encoded2, sizeof(encoded2));

    const struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res;

    if (getaddrinfo(TTS_SERVER, TTS_PORT, &hints, &res) != 0 || !res) {
        ESP_LOGE(TAG, "DNS failed");
        return ESP_FAIL;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    struct timeval tv = { .tv_sec = 10 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(res);
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    char post_data[1024];
    int post_len = snprintf(post_data, sizeof(post_data),
        "tex=%s&lan=zh&cuid=%s&ctp=1&aue=4&spd=5&pit=5&vol=9&per=0&tok=%s",
        encoded2, CUID, s_access_token);

    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "POST /text2audio HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n\r\n",
        TTS_SERVER, post_len);

    write(sock, header, header_len);
    write(sock, post_data, post_len);

    // Read response headers
    char recv_buf[512] = {0};
    int recv_len = 0;
    while (recv_len < (int)sizeof(recv_buf) - 1) {
        int n = read(sock, recv_buf + recv_len, 1);
        if (n <= 0) break;
        recv_len++;
        if (recv_len >= 4 && strncmp(recv_buf + recv_len - 4, "\r\n\r\n", 4) == 0)
            break;
    }

    if (!strstr(recv_buf, "audio")) {
        ESP_LOGE(TAG, "TTS error: %s", recv_buf);
        close(sock);
        return ESP_FAIL;
    }

    int content_length = -1;
    char *cl = strstr(recv_buf, "Content-Length:");
    if (cl) content_length = atoi(cl + 15);

    // Save to file
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", PROMPT_PATH, filename);
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to create file: %s", filepath);
        close(sock);
        return ESP_FAIL;
    }

    uint8_t buf[1024];
    int total = 0;
    while (content_length < 0 || total < content_length) {
        int n = read(sock, buf, sizeof(buf));
        if (n <= 0) break;
        fwrite(buf, 1, n, fp);
        total += n;
    }

    fclose(fp);
    close(sock);
    ESP_LOGI(TAG, "Prompt saved: %s (%d bytes)", filepath, total);
    return ESP_OK;
}

// Play prompt from SD card (blocking, chunked like TTS)
esp_err_t baidu_tts_play_prompt(const char *filename)
{
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/%s", PROMPT_PATH, filename);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Prompt not found: %s", filepath);
        return ESP_FAIL;
    }

    // Allocate buffers (same as TTS streaming)
    uint8_t *read_buf = malloc(4096);
    int16_t *resample_buf = malloc(4096 * 3 * sizeof(int16_t));
    if (!read_buf || !resample_buf) {
        if (read_buf) free(read_buf);
        if (resample_buf) free(resample_buf);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    if (i2s_speaker_start() != ESP_OK) {
        free(read_buf);
        free(resample_buf);
        fclose(fp);
        return ESP_FAIL;
    }

    // Read and play in chunks
    while (1) {
        size_t n = fread(read_buf, 1, 4096, fp);
        if (n == 0) break;

        // Align to 2 bytes
        n &= ~1;
        if (n == 0) break;

        size_t in_samples = n / 2;
        size_t out_samples = resample_to_44k((int16_t *)read_buf, in_samples,
                                              resample_buf, 4096 * 3);
        i2s_speaker_play((uint8_t *)resample_buf, out_samples * 2);
    }

    fclose(fp);
    i2s_speaker_stop();
    free(read_buf);
    free(resample_buf);

    ESP_LOGI(TAG, "Prompt played: %s", filename);
    return ESP_OK;
}