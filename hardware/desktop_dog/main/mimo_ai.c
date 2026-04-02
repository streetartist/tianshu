/**
 * Mimo AI - Xiaomi Mimo API with tool calling support
 */

#include "mimo_ai.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "dht11.h"
#include "tianshu.h"
#include "audio_player.h"
#include "app_buffers.h"
#include "app_payloads.h"
#include "pet/pet_attributes.h"

static const char *TAG = "MIMO_AI";

#define MIMO_API_KEY "YOUR_MIMO_API_KEY"
#define MIMO_API_URL "https://api.xiaomimimo.com/v1/chat/completions"
#define WORLD_TIME_URL "https://uapis.cn/api/v1/misc/worldtime?city=Shanghai"

// JK Music API
#define JKAPI_MUSIC_URL "https://jkapi.com/api/music"
#define JKAPI_KEY "YOUR_JKAPI_KEY"

// Tool config IDs (cached)
static int s_weather_config_id = -1;
static int s_hitokoto_config_id = -1;

// Pending actions (execute after TTS completes)
typedef enum {
    PENDING_ACTION_NONE = 0,
    PENDING_ACTION_PLAY_MUSIC,
    PENDING_ACTION_STOP_MUSIC,
    PENDING_ACTION_CONTINUE_CHAT  // 连续对话
} pending_action_t;

static pending_action_t s_pending_action = PENDING_ACTION_NONE;
static char s_pending_music_url[512] = {0};

// Conversation context
#define MAX_CONTEXT_MESSAGES 10
typedef struct {
    char role[16];          // "user", "assistant", "tool"
    char content[512];
    // For tool calls
    bool has_tool_call;
    char tool_call_id[64];
    char tool_name[32];
    char tool_args[256];
} context_message_t;

static context_message_t s_context[MAX_CONTEXT_MESSAGES];
static int s_context_count = 0;

// HTTP response structure
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} http_response_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *resp = (http_response_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && resp) {
        if (resp->len + evt->data_len < resp->cap) {
            memcpy(resp->data + resp->len, evt->data, evt->data_len);
            resp->len += evt->data_len;
            resp->data[resp->len] = '\0';
        }
    }
    return ESP_OK;
}

// Get world time from API
static esp_err_t get_world_time(char *time_str, size_t size)
{
    if (!time_str || size == 0) return ESP_ERR_INVALID_ARG;
    time_str[0] = '\0';

    char *resp_buf = malloc(512);
    if (!resp_buf) return ESP_ERR_NO_MEM;

    http_response_t resp = {.data = resp_buf, .len = 0, .cap = 512};

    esp_http_client_config_t config = {
        .url = WORLD_TIME_URL,
        .timeout_ms = 5000,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        free(resp_buf);
        return ESP_FAIL;
    }

    cJSON *json = cJSON_Parse(resp_buf);
    if (json) {
        const char *datetime = cJSON_GetStringValue(cJSON_GetObjectItem(json, "datetime"));
        const char *weekday = cJSON_GetStringValue(cJSON_GetObjectItem(json, "weekday"));
        if (datetime && weekday) {
            snprintf(time_str, size, "%s %s", datetime, weekday);
        }
        cJSON_Delete(json);
    }

    free(resp_buf);
    return time_str[0] ? ESP_OK : ESP_FAIL;
}

// Get hitokoto quote
static esp_err_t get_hitokoto(char *quote, size_t size)
{
    if (!quote || size == 0) return ESP_ERR_INVALID_ARG;
    quote[0] = '\0';

    if (s_hitokoto_config_id < 0) {
        ts_api_config_t *configs = malloc(4 * sizeof(ts_api_config_t));
        if (!configs) return ESP_ERR_NO_MEM;

        int count = tianshu_gateway_get_configs(configs, 4);
        for (int i = 0; i < count; i++) {
            if (strstr(configs[i].name, "hitokoto") || strstr(configs[i].name, "格言")) {
                s_hitokoto_config_id = configs[i].id;
                break;
            }
        }
        free(configs);
    }

    if (s_hitokoto_config_id < 0) return ESP_ERR_NOT_FOUND;

    char result[512];
    if (tianshu_gateway_call(s_hitokoto_config_id, "{}", result, sizeof(result)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *json = cJSON_Parse(result);
    if (json) {
        const char *text = cJSON_GetStringValue(cJSON_GetObjectItem(json, "hitokoto"));
        if (text) {
            strncpy(quote, text, size - 1);
            quote[size - 1] = '\0';
        }
        cJSON_Delete(json);
    }

    return quote[0] ? ESP_OK : ESP_FAIL;
}

// Execute weather tool
static esp_err_t execute_weather_tool(const char *city, char *result, size_t size)
{
    if (!city || !result) return ESP_ERR_INVALID_ARG;
    result[0] = '\0';

    if (s_weather_config_id < 0) {
        ts_api_config_t *configs = malloc(4 * sizeof(ts_api_config_t));
        if (!configs) return ESP_ERR_NO_MEM;

        int count = tianshu_gateway_get_configs(configs, 4);
        for (int i = 0; i < count; i++) {
            if (strstr(configs[i].name, "weather") || strstr(configs[i].name, "天气")) {
                s_weather_config_id = configs[i].id;
                break;
            }
        }
        free(configs);
    }

    if (s_weather_config_id < 0) {
        snprintf(result, size, "天气服务未配置");
        return ESP_ERR_NOT_FOUND;
    }

    char params[128];
    snprintf(params, sizeof(params), "{\"city\":\"%s\"}", city);

    if (tianshu_gateway_call(s_weather_config_id, params, result, size) != ESP_OK) {
        snprintf(result, size, "获取天气失败");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// Case-insensitive strstr
static const char *my_strcasestr(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;

    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return haystack;
        }
    }
    return NULL;
}

// Filter out <think>...</think> tags from AI response
// Handles both "<think>...</think>" and "...</think>" formats
static void filter_think_tags(const char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return;

    const char *p = src;
    const char *end_tag;

    // First, check if there's a </think> tag (AI might omit opening <think>)
    while ((end_tag = my_strcasestr(p, "</think>")) != NULL) {
        // Skip everything before and including </think>
        p = end_tag + 8;
    }

    // Now p points to content after last </think>, or original src if no tag
    // Trim leading whitespace/newlines
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
        p++;
    }

    // Copy remaining content
    strncpy(dst, p, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

// URL encode helper
static void url_encode(const char *src, char *dst, size_t dst_size)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 4; i++) {
        unsigned char c = (unsigned char)src[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[j++] = c;
        } else {
            dst[j++] = '%';
            dst[j++] = hex[c >> 4];
            dst[j++] = hex[c & 0x0F];
        }
    }
    dst[j] = '\0';
}

// Execute music tool - search and play music via JK API
static esp_err_t execute_music_tool(const char *song_name, char *result, size_t size)
{
    if (!song_name || !result) return ESP_ERR_INVALID_ARG;
    result[0] = '\0';

    ESP_LOGI(TAG, "Searching music: %s", song_name);

    // URL encode the song name
    char encoded_name[256];
    url_encode(song_name, encoded_name, sizeof(encoded_name));

    // Build URL with query params
    char url[512];
    snprintf(url, sizeof(url),
        "%s?plat=qq&type=json&apiKey=%s&name=%s",
        JKAPI_MUSIC_URL, JKAPI_KEY, encoded_name);

    // Allocate response buffer
    char *resp_buf = malloc(2048);
    if (!resp_buf) return ESP_ERR_NO_MEM;
    http_response_t resp = {.data = resp_buf, .len = 0, .cap = 2048};

    // HTTP GET request
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "Music API error: %d, status: %d", err, status);
        snprintf(result, size, "搜索音乐失败");
        free(resp_buf);
        return ESP_FAIL;
    }

    // Parse response
    cJSON *json = cJSON_Parse(resp_buf);
    free(resp_buf);

    if (!json) {
        snprintf(result, size, "解析失败");
        return ESP_FAIL;
    }

    // Check code
    int code = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "code"));
    if (code != 1) {
        cJSON_Delete(json);
        snprintf(result, size, "未找到歌曲");
        return ESP_FAIL;
    }

    // Extract track info
    const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(json, "name"));
    const char *artist = cJSON_GetStringValue(cJSON_GetObjectItem(json, "artist"));
    const char *music_url = cJSON_GetStringValue(cJSON_GetObjectItem(json, "music_url"));

    if (!music_url) {
        cJSON_Delete(json);
        snprintf(result, size, "获取播放链接失败");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Found: %s - %s", name ? name : "?", artist ? artist : "?");

    // Save URL for later playback (after AI response)
    strncpy(s_pending_music_url, music_url, sizeof(s_pending_music_url) - 1);
    s_pending_music_url[sizeof(s_pending_music_url) - 1] = '\0';

    // Format result
    snprintf(result, size, "正在播放: %s - %s",
             name ? name : "未知", artist ? artist : "未知");

    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t mimo_ai_init(void)
{
    s_context_count = 0;
    ESP_LOGI(TAG, "Mimo AI initialized");
    return ESP_OK;
}

// Add message to context
static void add_to_context(const char *role, const char *content)
{
    if (s_context_count >= MAX_CONTEXT_MESSAGES) {
        // Remove oldest message (shift array)
        memmove(&s_context[0], &s_context[1],
                (MAX_CONTEXT_MESSAGES - 1) * sizeof(context_message_t));
        s_context_count = MAX_CONTEXT_MESSAGES - 1;
    }
    memset(&s_context[s_context_count], 0, sizeof(context_message_t));
    strncpy(s_context[s_context_count].role, role, sizeof(s_context[0].role) - 1);
    strncpy(s_context[s_context_count].content, content, sizeof(s_context[0].content) - 1);
    s_context_count++;
}

// Add tool call to context (assistant message with tool_calls)
static void add_tool_call_to_context(const char *tool_call_id, const char *name, const char *args)
{
    if (s_context_count >= MAX_CONTEXT_MESSAGES) {
        memmove(&s_context[0], &s_context[1],
                (MAX_CONTEXT_MESSAGES - 1) * sizeof(context_message_t));
        s_context_count = MAX_CONTEXT_MESSAGES - 1;
    }
    memset(&s_context[s_context_count], 0, sizeof(context_message_t));
    strncpy(s_context[s_context_count].role, "assistant", 15);
    s_context[s_context_count].has_tool_call = true;
    strncpy(s_context[s_context_count].tool_call_id, tool_call_id, 63);
    strncpy(s_context[s_context_count].tool_name, name, 31);
    strncpy(s_context[s_context_count].tool_args, args ? args : "{}", 255);
    s_context_count++;
}

// Add tool result to context
static void add_tool_result_to_context(const char *tool_call_id, const char *result)
{
    if (s_context_count >= MAX_CONTEXT_MESSAGES) {
        memmove(&s_context[0], &s_context[1],
                (MAX_CONTEXT_MESSAGES - 1) * sizeof(context_message_t));
        s_context_count = MAX_CONTEXT_MESSAGES - 1;
    }
    memset(&s_context[s_context_count], 0, sizeof(context_message_t));
    strncpy(s_context[s_context_count].role, "tool", 15);
    strncpy(s_context[s_context_count].tool_call_id, tool_call_id, 63);
    strncpy(s_context[s_context_count].content, result, sizeof(s_context[0].content) - 1);
    s_context_count++;
}

// Reset context
void mimo_ai_reset_context(void)
{
    s_context_count = 0;
    ESP_LOGI(TAG, "Context reset");
}

// Check if continue chat is pending
bool mimo_ai_is_continue_chat_pending(void)
{
    return s_pending_action == PENDING_ACTION_CONTINUE_CHAT;
}

// Execute pending actions (call after TTS completes without interruption)
esp_err_t mimo_ai_execute_pending_actions(void)
{
    esp_err_t ret = ESP_OK;

    switch (s_pending_action) {
        case PENDING_ACTION_PLAY_MUSIC:
            if (s_pending_music_url[0] != '\0') {
                ESP_LOGI(TAG, "Playing pending music: %s", s_pending_music_url);
                ret = audio_player_play_url(s_pending_music_url);
                s_pending_music_url[0] = '\0';
            }
            break;
        case PENDING_ACTION_STOP_MUSIC:
            ESP_LOGI(TAG, "Executing pending stop music");
            audio_player_stop();
            break;
        default:
            break;
    }

    s_pending_action = PENDING_ACTION_NONE;
    return ret;
}

// Clear pending actions (call when TTS is interrupted)
void mimo_ai_clear_pending_actions(void)
{
    s_pending_action = PENDING_ACTION_NONE;
    s_pending_music_url[0] = '\0';
    ESP_LOGI(TAG, "Pending actions cleared");
}

// Build tools JSON array
static cJSON *build_tools_json(void)
{
    cJSON *tools = cJSON_CreateArray();

    // Weather tool
    cJSON *weather = cJSON_CreateObject();
    cJSON_AddStringToObject(weather, "type", "function");
    cJSON *wfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(wfunc, "name", "get_weather");
    cJSON_AddStringToObject(wfunc, "description", "获取指定城市的天气预报");
    cJSON *wparams = cJSON_CreateObject();
    cJSON_AddStringToObject(wparams, "type", "object");
    cJSON *wprops = cJSON_CreateObject();
    cJSON *wcity = cJSON_CreateObject();
    cJSON_AddStringToObject(wcity, "type", "string");
    cJSON_AddStringToObject(wcity, "description", "城市名称");
    cJSON_AddItemToObject(wprops, "city", wcity);
    cJSON_AddItemToObject(wparams, "properties", wprops);
    cJSON *wreq = cJSON_CreateArray();
    cJSON_AddItemToArray(wreq, cJSON_CreateString("city"));
    cJSON_AddItemToObject(wparams, "required", wreq);
    cJSON_AddItemToObject(wfunc, "parameters", wparams);
    cJSON_AddItemToObject(weather, "function", wfunc);
    cJSON_AddItemToArray(tools, weather);

    // Music tool
    cJSON *music = cJSON_CreateObject();
    cJSON_AddStringToObject(music, "type", "function");
    cJSON *mfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(mfunc, "name", "play_music");
    cJSON_AddStringToObject(mfunc, "description", "搜索并播放音乐");
    cJSON *mparams = cJSON_CreateObject();
    cJSON_AddStringToObject(mparams, "type", "object");
    cJSON *mprops = cJSON_CreateObject();
    cJSON *msong = cJSON_CreateObject();
    cJSON_AddStringToObject(msong, "type", "string");
    cJSON_AddStringToObject(msong, "description", "歌曲名或歌手名");
    cJSON_AddItemToObject(mprops, "song", msong);
    cJSON_AddItemToObject(mparams, "properties", mprops);
    cJSON *mreq = cJSON_CreateArray();
    cJSON_AddItemToArray(mreq, cJSON_CreateString("song"));
    cJSON_AddItemToObject(mparams, "required", mreq);
    cJSON_AddItemToObject(mfunc, "parameters", mparams);
    cJSON_AddItemToObject(music, "function", mfunc);
    cJSON_AddItemToArray(tools, music);

    // Stop music tool
    cJSON *stop = cJSON_CreateObject();
    cJSON_AddStringToObject(stop, "type", "function");
    cJSON *sfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(sfunc, "name", "stop_music");
    cJSON_AddStringToObject(sfunc, "description", "停止当前播放的音乐");
    cJSON *sparams = cJSON_CreateObject();
    cJSON_AddStringToObject(sparams, "type", "object");
    cJSON_AddItemToObject(sparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(sfunc, "parameters", sparams);
    cJSON_AddItemToObject(stop, "function", sfunc);
    cJSON_AddItemToArray(tools, stop);

    // Set volume tool
    cJSON *vol = cJSON_CreateObject();
    cJSON_AddStringToObject(vol, "type", "function");
    cJSON *vfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(vfunc, "name", "set_volume");
    cJSON_AddStringToObject(vfunc, "description", "设置音量大小");
    cJSON *vparams = cJSON_CreateObject();
    cJSON_AddStringToObject(vparams, "type", "object");
    cJSON *vprops = cJSON_CreateObject();
    cJSON *vlevel = cJSON_CreateObject();
    cJSON_AddStringToObject(vlevel, "type", "integer");
    cJSON_AddStringToObject(vlevel, "description", "音量值(0-100)");
    cJSON_AddItemToObject(vprops, "volume", vlevel);
    cJSON_AddItemToObject(vparams, "properties", vprops);
    cJSON *vreq = cJSON_CreateArray();
    cJSON_AddItemToArray(vreq, cJSON_CreateString("volume"));
    cJSON_AddItemToObject(vparams, "required", vreq);
    cJSON_AddItemToObject(vfunc, "parameters", vparams);
    cJSON_AddItemToObject(vol, "function", vfunc);
    cJSON_AddItemToArray(tools, vol);

    // Reset context tool
    cJSON *reset = cJSON_CreateObject();
    cJSON_AddStringToObject(reset, "type", "function");
    cJSON *rfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(rfunc, "name", "reset_context");
    cJSON_AddStringToObject(rfunc, "description", "重置对话上下文，清除之前的对话记录");
    cJSON *rparams = cJSON_CreateObject();
    cJSON_AddStringToObject(rparams, "type", "object");
    cJSON_AddItemToObject(rparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(rfunc, "parameters", rparams);
    cJSON_AddItemToObject(reset, "function", rfunc);
    cJSON_AddItemToArray(tools, reset);

    // Continue chat tool
    cJSON *cont = cJSON_CreateObject();
    cJSON_AddStringToObject(cont, "type", "function");
    cJSON *cfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(cfunc, "name", "continue_chat");
    cJSON_AddStringToObject(cfunc, "description", "继续对话，让用户可以直接继续说话而不需要再次唤醒");
    cJSON *cparams = cJSON_CreateObject();
    cJSON_AddStringToObject(cparams, "type", "object");
    cJSON_AddItemToObject(cparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(cfunc, "parameters", cparams);
    cJSON_AddItemToObject(cont, "function", cfunc);
    cJSON_AddItemToArray(tools, cont);

    // Get pet status tool
    cJSON *pet_status = cJSON_CreateObject();
    cJSON_AddStringToObject(pet_status, "type", "function");
    cJSON *psfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(psfunc, "name", "get_pet_status");
    cJSON_AddStringToObject(psfunc, "description", "获取宠物的当前状态，包括快乐值、饥饿值、疲倦值和心情");
    cJSON *psparams = cJSON_CreateObject();
    cJSON_AddStringToObject(psparams, "type", "object");
    cJSON_AddItemToObject(psparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(psfunc, "parameters", psparams);
    cJSON_AddItemToObject(pet_status, "function", psfunc);
    cJSON_AddItemToArray(tools, pet_status);

    // Pet feed tool
    cJSON *pet_feed = cJSON_CreateObject();
    cJSON_AddStringToObject(pet_feed, "type", "function");
    cJSON *pffunc = cJSON_CreateObject();
    cJSON_AddStringToObject(pffunc, "name", "pet_feed");
    cJSON_AddStringToObject(pffunc, "description", "喂养宠物，减少饥饿值，增加快乐值");
    cJSON *pfparams = cJSON_CreateObject();
    cJSON_AddStringToObject(pfparams, "type", "object");
    cJSON_AddItemToObject(pfparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(pffunc, "parameters", pfparams);
    cJSON_AddItemToObject(pet_feed, "function", pffunc);
    cJSON_AddItemToArray(tools, pet_feed);

    // Pet play tool
    cJSON *pet_play = cJSON_CreateObject();
    cJSON_AddStringToObject(pet_play, "type", "function");
    cJSON *ppfunc = cJSON_CreateObject();
    cJSON_AddStringToObject(ppfunc, "name", "pet_play");
    cJSON_AddStringToObject(ppfunc, "description", "和宠物玩耍，增加快乐值，但会增加疲倦值");
    cJSON *ppparams = cJSON_CreateObject();
    cJSON_AddStringToObject(ppparams, "type", "object");
    cJSON_AddItemToObject(ppparams, "properties", cJSON_CreateObject());
    cJSON_AddItemToObject(ppfunc, "parameters", ppparams);
    cJSON_AddItemToObject(pet_play, "function", ppfunc);
    cJSON_AddItemToArray(tools, pet_play);

    return tools;
}

// Main chat function with tool calling
esp_err_t mimo_ai_chat(const char *question, dyn_string_t *answer)
{
    if (!answer) return ESP_ERR_INVALID_ARG;
    dyn_string_clear(answer);

    esp_err_t ret = ESP_FAIL;
    char *post_data = NULL;
    char *resp_buf = NULL;

    // Allocate response buffer
    resp_buf = malloc(8192);
    if (!resp_buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        return ESP_ERR_NO_MEM;
    }
    memset(resp_buf, 0, 8192);

    http_response_t resp = {.data = resp_buf, .len = 0, .cap = 8192};

    // Get context info
    char time_str[64] = {0};
    char hitokoto[256] = {0};
    get_world_time(time_str, sizeof(time_str));
    get_hitokoto(hitokoto, sizeof(hitokoto));

    // Get pet mood for personality
    pet_data_t *pet = app_get_pet_data();
    const char *mood_personality = "";
    switch ((pet_mood_t)pet->current_mood) {
        case PET_MOOD_HAPPY:
            mood_personality = "你现在心情很好，语气欢快活泼。";
            break;
        case PET_MOOD_SAD:
            mood_personality = "你现在有点不开心，语气略显低落。";
            break;
        case PET_MOOD_HUNGRY:
            mood_personality = "你现在有点饿，可能会偶尔提醒用户喂食。";
            break;
        case PET_MOOD_TIRED:
            mood_personality = "你现在有点累，语气慵懒。";
            break;
        case PET_MOOD_SICK:
            mood_personality = "你现在身体不太舒服，需要照顾。";
            break;
        case PET_MOOD_DEAD:
            mood_personality = "你已经死了，需要复活才能恢复正常。";
            break;
        default:
            mood_personality = "";
            break;
    }

    // Build system prompt
    char system_prompt[1024];
    snprintf(system_prompt, sizeof(system_prompt),
        "你是智能物联网设备上的AI助手，也是一个电子宠物。能回答问题和调用工具。"
        "回答要求：1.不要使用markdown格式（如**、#、`等符号）；"
        "2.回答简洁自然，不超过几百字。"
        "当前时间：%s。环境：温度%.1f°C，湿度%.1f%%。"
        "宠物状态：快乐%d，饥饿%d，疲倦%d。%s"
        "%s%s",
        time_str[0] ? time_str : "未知",
        dht11_get_temperature(), dht11_get_humidity(),
        pet->stats.happiness, pet->stats.hunger, pet->stats.fatigue,
        mood_personality,
        hitokoto[0] ? "今日格言：" : "",
        hitokoto[0] ? hitokoto : "");

    // Build request JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "mimo-v2-flash");

    cJSON *messages = cJSON_AddArrayToObject(root, "messages");

    // System message
    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content", system_prompt);
    cJSON_AddItemToArray(messages, sys_msg);

    // Add context messages (conversation history)
    for (int i = 0; i < s_context_count; i++) {
        cJSON *ctx_msg = cJSON_CreateObject();
        cJSON_AddStringToObject(ctx_msg, "role", s_context[i].role);

        if (s_context[i].has_tool_call) {
            // Assistant message with tool_calls
            cJSON *tc_array = cJSON_AddArrayToObject(ctx_msg, "tool_calls");
            cJSON *tc = cJSON_CreateObject();
            cJSON_AddStringToObject(tc, "id", s_context[i].tool_call_id);
            cJSON_AddStringToObject(tc, "type", "function");
            cJSON *func = cJSON_CreateObject();
            cJSON_AddStringToObject(func, "name", s_context[i].tool_name);
            cJSON_AddStringToObject(func, "arguments", s_context[i].tool_args);
            cJSON_AddItemToObject(tc, "function", func);
            cJSON_AddItemToArray(tc_array, tc);
        } else if (strcmp(s_context[i].role, "tool") == 0) {
            // Tool result message
            cJSON_AddStringToObject(ctx_msg, "tool_call_id", s_context[i].tool_call_id);
            cJSON_AddStringToObject(ctx_msg, "content", s_context[i].content);
        } else {
            // Normal user/assistant message
            cJSON_AddStringToObject(ctx_msg, "content", s_context[i].content);
        }
        cJSON_AddItemToArray(messages, ctx_msg);
    }

    // User message
    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    cJSON_AddStringToObject(user_msg, "content", question);
    cJSON_AddItemToArray(messages, user_msg);

    // Add tools
    cJSON *tools = build_tools_json();
    cJSON_AddItemToObject(root, "tools", tools);

    // Parameters
    cJSON_AddNumberToObject(root, "max_completion_tokens", 1000);
    cJSON_AddNumberToObject(root, "temperature", 0.7);
    cJSON_AddBoolToObject(root, "stream", false);

    // Thinking disabled
    cJSON *thinking = cJSON_CreateObject();
    cJSON_AddStringToObject(thinking, "type", "disabled");
    cJSON_AddItemToObject(root, "thinking", thinking);

    post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!post_data) {
        ESP_LOGE(TAG, "Failed to create JSON");
        free(resp_buf);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Request: %.200s...", post_data);

    // HTTP client config
    esp_http_client_config_t config = {
        .url = MIMO_API_URL,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "api-key", MIMO_API_KEY);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "HTTP status: %d, len: %d", status, resp.len);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "HTTP error: %d, status: %d", err, status);
        // 400错误可能是上下文太长，清除上下文
        if (status == 400) {
            ESP_LOGW(TAG, "Clearing context due to 400 error");
            mimo_ai_reset_context();
        }
        free(post_data);
        free(resp_buf);
        return ESP_FAIL;
    }

    // Parse response
    cJSON *json = cJSON_Parse(resp_buf);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse response");
        free(post_data);
        free(resp_buf);
        return ESP_FAIL;
    }

    cJSON *choices = cJSON_GetObjectItem(json, "choices");
    cJSON *first = choices ? cJSON_GetArrayItem(choices, 0) : NULL;
    cJSON *message = first ? cJSON_GetObjectItem(first, "message") : NULL;

    if (!message) {
        cJSON_Delete(json);
        free(post_data);
        free(resp_buf);
        return ESP_FAIL;
    }

    // Check for tool calls
    cJSON *tool_calls = cJSON_GetObjectItem(message, "tool_calls");
    // Get reasoning_content for thinking mode
    const char *reasoning_content = cJSON_GetStringValue(cJSON_GetObjectItem(message, "reasoning_content"));
    bool has_tool_call = false;

    if (tool_calls && cJSON_IsArray(tool_calls) && cJSON_GetArraySize(tool_calls) > 0) {
        has_tool_call = true;
        // Handle tool call
        cJSON *tc = cJSON_GetArrayItem(tool_calls, 0);
        cJSON *func = cJSON_GetObjectItem(tc, "function");
        const char *tool_call_id = cJSON_GetStringValue(cJSON_GetObjectItem(tc, "id"));
        const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(func, "name"));
        const char *args = cJSON_GetStringValue(cJSON_GetObjectItem(func, "arguments"));

        ESP_LOGI(TAG, "Tool call: %s, args: %s", name ? name : "null", args ? args : "null");
        if (reasoning_content) {
            ESP_LOGI(TAG, "Reasoning: %.100s...", reasoning_content);
        }

        char tool_result[1024] = {0};

        if (name && strcmp(name, "get_weather") == 0) {
            // 查询类工具：立即执行
            cJSON *args_json = cJSON_Parse(args);
            const char *city = args_json ?
                cJSON_GetStringValue(cJSON_GetObjectItem(args_json, "city")) : NULL;
            if (city) {
                execute_weather_tool(city, tool_result, sizeof(tool_result));
            }
            if (args_json) cJSON_Delete(args_json);
        }
        else if (name && strcmp(name, "play_music") == 0) {
            // 动作类工具：搜索音乐获取URL，但延迟播放
            cJSON *args_json = cJSON_Parse(args);
            const char *song = args_json ?
                cJSON_GetStringValue(cJSON_GetObjectItem(args_json, "song")) : NULL;
            if (song) {
                // execute_music_tool 只搜索并保存URL，不播放
                execute_music_tool(song, tool_result, sizeof(tool_result));
                if (s_pending_music_url[0]) {
                    s_pending_action = PENDING_ACTION_PLAY_MUSIC;
                }
            }
            if (args_json) cJSON_Delete(args_json);
        }
        else if (name && strcmp(name, "stop_music") == 0) {
            // 动作类工具：延迟执行
            s_pending_action = PENDING_ACTION_STOP_MUSIC;
            snprintf(tool_result, sizeof(tool_result), "已停止播放");
        }
        else if (name && strcmp(name, "set_volume") == 0) {
            // 音量调节：立即执行
            cJSON *args_json = cJSON_Parse(args);
            int vol = args_json ?
                (int)cJSON_GetNumberValue(cJSON_GetObjectItem(args_json, "volume")) : -1;
            if (vol >= 0 && vol <= 100) {
                audio_player_set_volume(vol);
                snprintf(tool_result, sizeof(tool_result), "音量已设置为%d", vol);
            } else {
                snprintf(tool_result, sizeof(tool_result), "无效的音量值");
            }
            if (args_json) cJSON_Delete(args_json);
        }
        else if (name && strcmp(name, "reset_context") == 0) {
            // 重置上下文：立即执行
            mimo_ai_reset_context();
            snprintf(tool_result, sizeof(tool_result), "对话上下文已重置");
        }
        else if (name && strcmp(name, "continue_chat") == 0) {
            // 连续对话：延迟执行
            s_pending_action = PENDING_ACTION_CONTINUE_CHAT;
            snprintf(tool_result, sizeof(tool_result), "已开启连续对话");
        }
        else if (name && strcmp(name, "get_pet_status") == 0) {
            // 获取宠物状态
            pet_data_t *pet = app_get_pet_data();
            const char *mood_str = pet_attributes_mood_name((pet_mood_t)pet->current_mood);
            snprintf(tool_result, sizeof(tool_result),
                "宠物状态: 快乐值=%d, 饥饿值=%d, 疲倦值=%d, 心情=%s",
                pet->stats.happiness, pet->stats.hunger, pet->stats.fatigue, mood_str);
        }
        else if (name && strcmp(name, "pet_feed") == 0) {
            // 喂养宠物
            pet_data_t *pet = app_get_pet_data();
            if (pet_attributes_is_dead(pet)) {
                snprintf(tool_result, sizeof(tool_result), "宠物已经死了，需要先复活");
            } else {
                pet_attributes_feed(pet, false);
                pet->current_mood = pet_attributes_calculate_mood(pet);
                snprintf(tool_result, sizeof(tool_result),
                    "已喂养宠物，当前饥饿值=%d，快乐值=%d", pet->stats.hunger, pet->stats.happiness);
            }
        }
        else if (name && strcmp(name, "pet_play") == 0) {
            // 和宠物玩耍
            pet_data_t *pet = app_get_pet_data();
            if (pet_attributes_is_dead(pet)) {
                snprintf(tool_result, sizeof(tool_result), "宠物已经死了，需要先复活");
            } else {
                pet_attributes_play(pet);
                pet->current_mood = pet_attributes_calculate_mood(pet);
                snprintf(tool_result, sizeof(tool_result),
                    "已和宠物玩耍，当前快乐值=%d，疲倦值=%d", pet->stats.happiness, pet->stats.fatigue);
            }
        }

        // Send tool result back to AI for final response
        if (tool_result[0] && tool_call_id) {
            ESP_LOGI(TAG, "Tool result: %s", tool_result);

            // Save tool call context: user -> assistant(tool_call) -> tool(result)
            add_to_context("user", question);
            add_tool_call_to_context(tool_call_id, name, args);
            add_tool_result_to_context(tool_call_id, tool_result);

            // Build follow-up request with tool result
            cJSON *root2 = cJSON_CreateObject();
            cJSON_AddStringToObject(root2, "model", "mimo-v2-flash");

            cJSON *msgs2 = cJSON_AddArrayToObject(root2, "messages");

            // System message
            cJSON *sys2 = cJSON_CreateObject();
            cJSON_AddStringToObject(sys2, "role", "system");
            cJSON_AddStringToObject(sys2, "content", system_prompt);
            cJSON_AddItemToArray(msgs2, sys2);

            // User message
            cJSON *usr2 = cJSON_CreateObject();
            cJSON_AddStringToObject(usr2, "role", "user");
            cJSON_AddStringToObject(usr2, "content", question);
            cJSON_AddItemToArray(msgs2, usr2);

            // Assistant message with tool_calls and reasoning_content
            cJSON *asst2 = cJSON_CreateObject();
            cJSON_AddStringToObject(asst2, "role", "assistant");
            cJSON_AddNullToObject(asst2, "content");
            if (reasoning_content) {
                cJSON_AddStringToObject(asst2, "reasoning_content", reasoning_content);
            }
            cJSON *tcs2 = cJSON_AddArrayToObject(asst2, "tool_calls");

            // Add the tool call object
            cJSON *tc2 = cJSON_CreateObject();
            cJSON_AddStringToObject(tc2, "id", tool_call_id);
            cJSON_AddStringToObject(tc2, "type", "function");
            cJSON *func2 = cJSON_CreateObject();
            cJSON_AddStringToObject(func2, "name", name);
            cJSON_AddStringToObject(func2, "arguments", args);
            cJSON_AddItemToObject(tc2, "function", func2);
            cJSON_AddItemToArray(tcs2, tc2);
            cJSON_AddItemToArray(msgs2, asst2);

            // Tool message with result
            cJSON *tool_msg = cJSON_CreateObject();
            cJSON_AddStringToObject(tool_msg, "role", "tool");
            cJSON_AddStringToObject(tool_msg, "tool_call_id", tool_call_id);
            cJSON_AddStringToObject(tool_msg, "content", tool_result);
            cJSON_AddItemToArray(msgs2, tool_msg);

            // Add tools (AI needs to know available tools)
            cJSON *tools2 = build_tools_json();
            cJSON_AddItemToObject(root2, "tools", tools2);

            // Parameters
            cJSON_AddNumberToObject(root2, "max_completion_tokens", 500);
            cJSON_AddNumberToObject(root2, "temperature", 0.7);
            cJSON_AddBoolToObject(root2, "stream", false);

            char *post_data2 = cJSON_PrintUnformatted(root2);
            cJSON_Delete(root2);

            if (post_data2) {
                ESP_LOGI(TAG, "Tool follow-up request: %.200s...", post_data2);

                // Reset response buffer
                resp.len = 0;
                resp.data[0] = '\0';

                // Send follow-up request
                esp_http_client_handle_t client2 = esp_http_client_init(&config);
                esp_http_client_set_header(client2, "Content-Type", "application/json");
                esp_http_client_set_header(client2, "api-key", MIMO_API_KEY);
                esp_http_client_set_post_field(client2, post_data2, strlen(post_data2));

                esp_err_t err2 = esp_http_client_perform(client2);
                int status2 = esp_http_client_get_status_code(client2);
                esp_http_client_cleanup(client2);
                free(post_data2);

                ESP_LOGI(TAG, "Follow-up HTTP status: %d, len: %d", status2, resp.len);

                if (err2 == ESP_OK && status2 == 200) {
                    cJSON *json2 = cJSON_Parse(resp.data);
                    if (json2) {
                        cJSON *choices2 = cJSON_GetObjectItem(json2, "choices");
                        cJSON *first2 = choices2 ? cJSON_GetArrayItem(choices2, 0) : NULL;
                        cJSON *msg2 = first2 ? cJSON_GetObjectItem(first2, "message") : NULL;
                        cJSON *content2 = msg2 ? cJSON_GetObjectItem(msg2, "content") : NULL;

                        if (content2 && content2->valuestring) {
                            // Filter out <think> tags
                            size_t src_len = strlen(content2->valuestring);
                            char *filtered = malloc(src_len + 1);
                            if (filtered) {
                                filter_think_tags(content2->valuestring, filtered, src_len + 1);
                                dyn_string_append(answer, filtered);
                                free(filtered);
                            } else {
                                dyn_string_append(answer, content2->valuestring);
                            }
                            ret = ESP_OK;
                            ESP_LOGI(TAG, "Final AI Answer: %s", dyn_string_cstr(answer));
                            // Save final answer to context
                            add_to_context("assistant", dyn_string_cstr(answer));
                        }
                        cJSON_Delete(json2);
                    }
                }

                // Fallback to tool result if follow-up failed
                if (ret != ESP_OK && tool_result[0]) {
                    dyn_string_append(answer, tool_result);
                    ret = ESP_OK;
                }
            }
        } else if (tool_result[0]) {
            // No tool_call_id, just return tool result
            dyn_string_append(answer, tool_result);
            ret = ESP_OK;
        }
    }
    else {
        // No tool call, get content directly
        cJSON *content = cJSON_GetObjectItem(message, "content");
        if (content && content->valuestring) {
            // Filter out <think> tags
            size_t src_len = strlen(content->valuestring);
            char *filtered = malloc(src_len + 1);
            if (filtered) {
                filter_think_tags(content->valuestring, filtered, src_len + 1);
                dyn_string_append(answer, filtered);
                free(filtered);
            } else {
                dyn_string_append(answer, content->valuestring);
            }
            ret = ESP_OK;
            ESP_LOGI(TAG, "AI Answer: %s", dyn_string_cstr(answer));
        }
    }

    // Cleanup
    cJSON_Delete(json);
    free(post_data);
    free(resp_buf);

    // Save to context if successful and no tool call
    // (tool call context is saved earlier with complete chain)
    if (ret == ESP_OK && dyn_string_len(answer) > 0 && !has_tool_call) {
        add_to_context("user", question);
        add_to_context("assistant", dyn_string_cstr(answer));
    }

    return ret;
}
