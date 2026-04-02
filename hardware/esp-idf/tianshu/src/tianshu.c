/**
 * TianShu IoT SDK for ESP-IDF
 */

#include "tianshu.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "tianshu";

// Internal state
static ts_config_t s_config;
static ts_status_t s_status = TS_STATUS_DISCONNECTED;
static bool s_initialized = false;

#define TIANSHU_URL_BUF_SIZE 256
#define TIANSHU_HTTP_TIMEOUT_MS 10000
#define TIANSHU_HTTP_BUF_SIZE 1024

// WebSocket state
static esp_websocket_client_handle_t s_ws_client = NULL;
static bool s_ws_connected = false;

// HTTP响应收集结构
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} http_resp_t;

// HTTP事件处理器
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_resp_t *resp = (http_resp_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && resp && resp->data) {
        if (resp->len + evt->data_len < resp->cap) {
            memcpy(resp->data + resp->len, evt->data, evt->data_len);
            resp->len += evt->data_len;
            resp->data[resp->len] = '\0';
        }
    }
    return ESP_OK;
}

static esp_err_t http_post_json(const char *url, const char *json_body, char *resp_buf, size_t resp_buf_size, int *http_status)
{
    if (!url || !json_body) {
        return ESP_ERR_INVALID_ARG;
    }

    http_resp_t resp = {
        .data = resp_buf,
        .len = 0,
        .cap = resp_buf_size
    };
    if (resp_buf) resp_buf[0] = '\0';

    esp_http_client_config_t http_config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = TIANSHU_HTTP_TIMEOUT_MS,
        .buffer_size = TIANSHU_HTTP_BUF_SIZE,
        .buffer_size_tx = TIANSHU_HTTP_BUF_SIZE,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    if (http_status) {
        *http_status = status;
    }

    esp_http_client_cleanup(client);
    return (err == ESP_OK && status == 200) ? ESP_OK : ESP_FAIL;
}

static esp_err_t http_get(const char *url, char *resp_buf, size_t resp_buf_size, int *http_status)
{
    if (!url) return ESP_ERR_INVALID_ARG;

    http_resp_t resp = {
        .data = resp_buf,
        .len = 0,
        .cap = resp_buf_size
    };
    if (resp_buf) resp_buf[0] = '\0';

    esp_http_client_config_t http_config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = TIANSHU_HTTP_TIMEOUT_MS,
        .buffer_size = TIANSHU_HTTP_BUF_SIZE,
        .event_handler = http_event_handler,
        .user_data = &resp,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) return ESP_ERR_NO_MEM;

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    if (http_status) *http_status = status;

    esp_http_client_cleanup(client);
    return (err == ESP_OK && status == 200) ? ESP_OK : ESP_FAIL;
}

esp_err_t tianshu_init(const ts_config_t *config)
{
    if (!config || !config->server_url || !config->device_id || !config->secret_key) {
        ESP_LOGE(TAG, "Invalid config");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_config, config, sizeof(ts_config_t));

    if (s_config.heartbeat_interval == 0) {
        s_config.heartbeat_interval = 60;
    }
    if (s_config.data_interval == 0) {
        s_config.data_interval = 300;
    }

    ESP_LOGI(TAG, "TianShu SDK v%s initialized", TIANSHU_VERSION);
    ESP_LOGI(TAG, "Device ID: %s", s_config.device_id);
    s_initialized = true;

    return ESP_OK;
}

ts_status_t tianshu_get_status(void)
{
    return s_status;
}

esp_err_t tianshu_connect(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_status = TS_STATUS_CONNECTING;

    esp_err_t ret = tianshu_heartbeat();
    if (ret == ESP_OK) {
        s_status = TS_STATUS_CONNECTED;
        ESP_LOGI(TAG, "Connected to server");
    } else {
        s_status = TS_STATUS_ERROR;
        ESP_LOGE(TAG, "Connection failed");
    }

    return ret;
}

void tianshu_disconnect(void)
{
    s_status = TS_STATUS_DISCONNECTED;
    ESP_LOGI(TAG, "Disconnected");
}

esp_err_t tianshu_heartbeat(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    char url[TIANSHU_URL_BUF_SIZE];
    int url_len = snprintf(url, sizeof(url), "%s/api/v1/devices/heartbeat", s_config.server_url);
    if (url_len <= 0 || url_len >= (int)sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(root, "device_id", s_config.device_id);
    cJSON_AddStringToObject(root, "secret_key", s_config.secret_key);
    char *post_data = cJSON_PrintUnformatted(root);
    if (!post_data) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_post_json(url, post_data, NULL, 0, &status);

    cJSON_Delete(root);
    free(post_data);

    s_status = (err == ESP_OK) ? TS_STATUS_CONNECTED : TS_STATUS_ERROR;
    return err;
}

esp_err_t tianshu_report(const char *name, ts_data_type_t type, void *value)
{
    if (!name || !value) {
        return ESP_ERR_INVALID_ARG;
    }

    ts_datapoint_t point = { .name = name, .type = type };

    switch (type) {
        case TS_TYPE_INT:
            point.value.i = *(int32_t *)value;
            break;
        case TS_TYPE_FLOAT:
            point.value.f = *(float *)value;
            break;
        case TS_TYPE_BOOL:
            point.value.b = *(bool *)value;
            break;
        case TS_TYPE_STRING:
            point.value.s = (const char *)value;
            break;
        case TS_TYPE_JSON:
            point.value.s = (const char *)value;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    return tianshu_report_batch(&point, 1);
}

esp_err_t tianshu_report_batch(ts_datapoint_t *points, size_t count)
{
    if (!points || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    char url[TIANSHU_URL_BUF_SIZE];
    int url_len = snprintf(url, sizeof(url), "%s/api/v1/data/report", s_config.server_url);
    if (url_len <= 0 || url_len >= (int)sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(root, "device_id", s_config.device_id);
    cJSON_AddStringToObject(root, "secret_key", s_config.secret_key);

    cJSON *data = cJSON_CreateObject();
    if (!data) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    for (size_t i = 0; i < count; i++) {
        if (!points[i].name) {
            ESP_LOGW(TAG, "Data point name missing");
            continue;
        }
        switch (points[i].type) {
            case TS_TYPE_INT:
                cJSON_AddNumberToObject(data, points[i].name, points[i].value.i);
                break;
            case TS_TYPE_FLOAT:
                cJSON_AddNumberToObject(data, points[i].name, points[i].value.f);
                break;
            case TS_TYPE_BOOL:
                cJSON_AddBoolToObject(data, points[i].name, points[i].value.b);
                break;
            case TS_TYPE_STRING:
                if (points[i].value.s) {
                    cJSON_AddStringToObject(data, points[i].name, points[i].value.s);
                } else {
                    ESP_LOGW(TAG, "String value missing for %s", points[i].name);
                }
                break;
            case TS_TYPE_JSON: {
                if (points[i].value.s) {
                    cJSON *json_item = cJSON_Parse(points[i].value.s);
                    if (json_item) {
                        cJSON_AddItemToObject(data, points[i].name, json_item);
                    } else {
                        ESP_LOGW(TAG, "Invalid JSON for %s", points[i].name);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    cJSON_AddItemToObject(root, "data", data);

    char *post_data = cJSON_PrintUnformatted(root);
    if (!post_data) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_post_json(url, post_data, NULL, 0, &status);

    cJSON_Delete(root);
    free(post_data);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Data reported: %zu points", count);
        s_status = TS_STATUS_CONNECTED;
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Report failed: %d", status);
    s_status = TS_STATUS_ERROR;
    return err;
}

// Socket.IO 认证状态
static bool s_sio_connected = false;

// 消息缓冲区（处理分片）
#define WS_MSG_BUF_SIZE 4096
static char s_msg_buf[WS_MSG_BUF_SIZE];
static int s_msg_len = 0;

// 发送设备认证
static void send_device_auth(void)
{
    if (!s_ws_client) {
        return;
    }

    cJSON *auth = cJSON_CreateObject();
    if (!auth) {
        return;
    }
    cJSON_AddStringToObject(auth, "device_id", s_config.device_id);
    cJSON_AddStringToObject(auth, "secret_key", s_config.secret_key);
    char *auth_str = cJSON_PrintUnformatted(auth);
    if (!auth_str) {
        cJSON_Delete(auth);
        return;
    }

    size_t msg_len = strlen(auth_str) + strlen("42[\"auth\",]") + 1;
    char *msg = malloc(msg_len);
    if (msg) {
        snprintf(msg, msg_len, "42[\"auth\",%s]", auth_str);
        ESP_LOGI(TAG, "Sending auth");
        esp_websocket_client_send_text(s_ws_client, msg, strlen(msg), portMAX_DELAY);
        free(msg);
    }

    free(auth_str);
    cJSON_Delete(auth);
}

// WebSocket event handler
static void ws_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket connected, waiting for Socket.IO handshake...");
        s_ws_connected = true;
        s_sio_connected = false;
        s_msg_len = 0;
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket disconnected");
        s_ws_connected = false;
        s_sio_connected = false;
        s_msg_len = 0;
        break;

    case WEBSOCKET_EVENT_DATA:
        if (data->op_code != 0x01) {
            break;  // 只处理文本帧
        }

        // 处理分片：累积数据到缓冲区
        if (data->payload_offset == 0) {
            s_msg_len = 0;  // 新消息开始，重置缓冲区
            if (data->payload_len >= WS_MSG_BUF_SIZE) {
                ESP_LOGE(TAG, "Message too large (%d), dropping", data->payload_len);
                break;
            }
        }

        // 复制数据到缓冲区
        if (s_msg_len + data->data_len <= WS_MSG_BUF_SIZE - 1) {
            memcpy(s_msg_buf + s_msg_len, data->data_ptr, data->data_len);
            s_msg_len += data->data_len;
        } else {
            ESP_LOGE(TAG, "Message too large, dropping");
            s_msg_len = 0;
            break;
        }

        // 检查是否收到完整消息
        if (data->payload_offset + data->data_len < data->payload_len) {
            ESP_LOGD(TAG, "Fragment received, waiting for more...");
            break;  // 等待更多分片
        }

        // 完整消息已收到，添加 null terminator
        s_msg_buf[s_msg_len] = '\0';
        ESP_LOGD(TAG, "WS recv[%d]: %s", s_msg_len, s_msg_buf);

        // 处理 Socket.IO 消息
        if (s_msg_len < 1) {
            break;
        }

        // Socket.IO OPEN packet (0{...})
        if (s_msg_buf[0] == '0') {
            ESP_LOGI(TAG, "Socket.IO OPEN received, sending CONNECT");
            esp_websocket_client_send_text(s_ws_client, "40", 2, portMAX_DELAY);
        }
        // Socket.IO PING (2) -> PONG (3)
        else if (s_msg_buf[0] == '2') {
            ESP_LOGI(TAG, "Socket.IO PING, sending PONG");
            esp_websocket_client_send_text(s_ws_client, "3", 1, portMAX_DELAY);
        }
        // Socket.IO CONNECT confirmation (40...)
        else if (s_msg_len >= 2 && s_msg_buf[0] == '4' && s_msg_buf[1] == '0') {
            ESP_LOGI(TAG, "Socket.IO CONNECT confirmed, sending auth");
            send_device_auth();
        }
        // Socket.IO MESSAGE (42[...])
        else if (s_msg_len >= 2 && s_msg_buf[0] == '4' && s_msg_buf[1] == '2') {
            const char *cmd_prefix = "42[\"command\"";
            const char *auth_prefix = "42[\"auth_result\"";

            if (strncmp(s_msg_buf, cmd_prefix, strlen(cmd_prefix)) != 0 &&
                strncmp(s_msg_buf, auth_prefix, strlen(auth_prefix)) != 0) {
                ESP_LOGD(TAG, "Socket.IO event ignored");
                s_msg_len = 0;
                break;
            }

            char *json_start = strchr(s_msg_buf, '[');
            if (!json_start) {
                ESP_LOGW(TAG, "No JSON array found in message");
                break;
            }

            cJSON *root = cJSON_Parse(json_start);
            if (!root) {
                ESP_LOGW(TAG, "Failed to parse JSON array");
                break;
            }

            if (cJSON_IsArray(root) && cJSON_GetArraySize(root) >= 2) {
                cJSON *event_item = cJSON_GetArrayItem(root, 0);
                cJSON *data_item = cJSON_GetArrayItem(root, 1);

                if (cJSON_IsString(event_item)) {
                    const char *event_name = event_item->valuestring;

                    // auth_result
                    if (strcmp(event_name, "auth_result") == 0) {
                        cJSON *code_item = cJSON_IsObject(data_item) ? cJSON_GetObjectItem(data_item, "code") : NULL;
                        int code = code_item ? code_item->valueint : -1;
                        const char *msg = cJSON_IsObject(data_item)
                            ? cJSON_GetStringValue(cJSON_GetObjectItem(data_item, "message"))
                            : NULL;
                        if (code == 0) {
                            s_sio_connected = true;
                            ESP_LOGI(TAG, "Auth success: %s", msg ? msg : "OK");
                        } else {
                            s_sio_connected = false;
                            ESP_LOGE(TAG, "Auth failed: %d, %s", code, msg ? msg : "");
                        }
                    }
                    // command
                    else if (strcmp(event_name, "command") == 0) {
                        if (s_config.command_callback) {
                            const char *cmd_name = cJSON_GetStringValue(cJSON_GetObjectItem(data_item, "command"));
                            cJSON *params = cJSON_GetObjectItem(data_item, "params");
                            char *params_str = params ? cJSON_PrintUnformatted(params) : NULL;
                            
                            if (cmd_name) {
                                ESP_LOGI(TAG, "Executing command: %s", cmd_name);
                                s_config.command_callback(cmd_name, params_str ? params_str : "{}");
                            } else {
                                ESP_LOGW(TAG, "Command name missing");
                            }
                            
                            if (params_str) free(params_str);
                        }
                    }
                }
            } else {
                ESP_LOGW(TAG, "Invalid Socket.IO packet format");
            }
            cJSON_Delete(root);
        }

        s_msg_len = 0;  // 重置缓冲区
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket error");
        s_sio_connected = false;
        s_msg_len = 0;
        break;
    }
}

esp_err_t tianshu_ws_connect(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ws_client) {
        return ESP_OK;
    }

    char ws_url[TIANSHU_URL_BUF_SIZE];
    int url_len = 0;
    if (strncmp(s_config.server_url, "http://", 7) == 0) {
        url_len = snprintf(ws_url, sizeof(ws_url), "ws://%s/socket.io/?EIO=4&transport=websocket",
                           s_config.server_url + 7);
    } else if (strncmp(s_config.server_url, "https://", 8) == 0) {
        url_len = snprintf(ws_url, sizeof(ws_url), "wss://%s/socket.io/?EIO=4&transport=websocket",
                           s_config.server_url + 8);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    if (url_len <= 0 || url_len >= (int)sizeof(ws_url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "WS connecting to: %s", ws_url);

    esp_websocket_client_config_t ws_cfg = {
        .uri = ws_url,
        .buffer_size = WS_MSG_BUF_SIZE,    // 接收缓冲区
        .ping_interval_sec = 10,           // WebSocket 层 ping 保活
        .network_timeout_ms = 10000,       // 网络超时
        .reconnect_timeout_ms = 5000,      // 重连间隔
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    s_ws_client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(s_ws_client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);

    return esp_websocket_client_start(s_ws_client);
}

void tianshu_ws_disconnect(void)
{
    if (s_ws_client) {
        esp_websocket_client_stop(s_ws_client);
        esp_websocket_client_destroy(s_ws_client);
        s_ws_client = NULL;
        s_ws_connected = false;
        s_sio_connected = false;
        s_msg_len = 0;
    }
}

bool tianshu_ws_is_connected(void)
{
    return s_ws_client && s_ws_connected && esp_websocket_client_is_connected(s_ws_client) && s_sio_connected;
}

esp_err_t tianshu_ws_send_cmd_result(const char *cmd_id, bool success, const char *result)
{
    if (!cmd_id) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!tianshu_ws_is_connected() || !s_ws_client) {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *data = cJSON_CreateObject();
    if (!data) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(data, "device_id", s_config.device_id);
    cJSON_AddStringToObject(data, "cmd_id", cmd_id);
    cJSON_AddBoolToObject(data, "success", success);
    if (result) {
        cJSON_AddStringToObject(data, "result", result);
    }
    char *data_str = cJSON_PrintUnformatted(data);
    if (!data_str) {
        cJSON_Delete(data);
        return ESP_ERR_NO_MEM;
    }

    size_t msg_len = strlen(data_str) + strlen("42[\"cmd_result\",]") + 1;
    char *msg = malloc(msg_len);
    if (!msg) {
        free(data_str);
        cJSON_Delete(data);
        return ESP_ERR_NO_MEM;
    }

    snprintf(msg, msg_len, "42[\"cmd_result\",%s]", data_str);
    esp_websocket_client_send_text(s_ws_client, msg, strlen(msg), portMAX_DELAY);

    free(msg);
    free(data_str);
    cJSON_Delete(data);

    return ESP_OK;
}

esp_err_t tianshu_get_config(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    buf[0] = '\0';
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    char url[TIANSHU_URL_BUF_SIZE];
    int url_len = snprintf(url, sizeof(url), "%s/api/v1/config/pull", s_config.server_url);
    if (url_len <= 0 || url_len >= (int)sizeof(url)) {
        return ESP_ERR_INVALID_SIZE;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(root, "device_id", s_config.device_id);
    cJSON_AddStringToObject(root, "secret_key", s_config.secret_key);
    char *post_data = cJSON_PrintUnformatted(root);
    if (!post_data) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_post_json(url, post_data, buf, buf_size, &status);

    cJSON_Delete(root);
    free(post_data);

    if (err != ESP_OK) {
        return err;
    }

    cJSON *resp = cJSON_Parse(buf);
    if (!resp) {
        return ESP_FAIL;
    }
    cJSON *code_item = cJSON_GetObjectItem(resp, "code");
    int code = cJSON_IsNumber(code_item) ? code_item->valueint : -1;
    cJSON_Delete(resp);

    return code == 0 ? ESP_OK : ESP_FAIL;
}

// ============ API Gateway Implementation ============

static ts_api_type_t parse_api_type(const char *type_str)
{
    if (!type_str) return TS_API_TYPE_AI_MODEL;
    if (strcmp(type_str, "python_handler") == 0) return TS_API_TYPE_PYTHON_HANDLER;
    if (strcmp(type_str, "normal_api") == 0) return TS_API_TYPE_NORMAL_API;
    return TS_API_TYPE_AI_MODEL;
}

static ts_proxy_mode_t parse_proxy_mode(const char *mode_str)
{
    if (!mode_str) return TS_PROXY_SERVER;
    if (strcmp(mode_str, "direct") == 0) return TS_PROXY_DIRECT;
    return TS_PROXY_SERVER;
}

int tianshu_gateway_get_configs(ts_api_config_t *configs, int max_count)
{
    if (!configs || max_count <= 0 || !s_initialized) {
        ESP_LOGW(TAG, "get_configs: invalid args or not initialized");
        return 0;
    }

    char url[TIANSHU_URL_BUF_SIZE];
    snprintf(url, sizeof(url), "%s/api/v1/gateway/device/configs?device_id=%s&secret_key=%s",
        s_config.server_url, s_config.device_id, s_config.secret_key);

    ESP_LOGI(TAG, "get_configs URL: %s", url);

    char *resp_buf = malloc(4096);
    if (!resp_buf) return 0;

    int status = 0;
    esp_err_t err = http_get(url, resp_buf, 4096, &status);
    ESP_LOGI(TAG, "get_configs: err=%d, status=%d", err, status);
    if (err != ESP_OK || status != 200) {
        ESP_LOGW(TAG, "get_configs failed: err=%d, status=%d", err, status);
        free(resp_buf);
        return 0;
    }

    int count = 0;
    cJSON *json = cJSON_Parse(resp_buf);
    if (!json) {
        free(resp_buf);
        return 0;
    }

    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data && cJSON_IsArray(data)) {
        int arr_size = cJSON_GetArraySize(data);
        for (int i = 0; i < arr_size && count < max_count; i++) {
            cJSON *item = cJSON_GetArrayItem(data, i);
            ts_api_config_t *cfg = &configs[count];
            memset(cfg, 0, sizeof(ts_api_config_t));

            cfg->id = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(item, "id"));
            cfg->timeout = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(item, "timeout"));

            const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(item, "name"));
            if (name) strncpy(cfg->name, name, sizeof(cfg->name) - 1);

            cfg->api_type = parse_api_type(cJSON_GetStringValue(cJSON_GetObjectItem(item, "api_type")));
            cfg->proxy_mode = parse_proxy_mode(cJSON_GetStringValue(cJSON_GetObjectItem(item, "proxy_mode")));

            const char *base_url = cJSON_GetStringValue(cJSON_GetObjectItem(item, "base_url"));
            if (base_url) strncpy(cfg->base_url, base_url, sizeof(cfg->base_url) - 1);

            const char *api_key = cJSON_GetStringValue(cJSON_GetObjectItem(item, "api_key"));
            if (api_key) strncpy(cfg->api_key, api_key, sizeof(cfg->api_key) - 1);

            const char *method = cJSON_GetStringValue(cJSON_GetObjectItem(item, "request_method"));
            if (method) strncpy(cfg->request_method, method, sizeof(cfg->request_method) - 1);

            const char *tpl = cJSON_GetStringValue(cJSON_GetObjectItem(item, "request_body_template"));
            if (tpl) strncpy(cfg->request_body_template, tpl, sizeof(cfg->request_body_template) - 1);

            const char *extract = cJSON_GetStringValue(cJSON_GetObjectItem(item, "response_extract"));
            if (extract) strncpy(cfg->response_extract, extract, sizeof(cfg->response_extract) - 1);

            count++;
        }
    }

    cJSON_Delete(json);
    free(resp_buf);
    return count;
}

esp_err_t tianshu_gateway_call(int config_id, const char *params_json, char *result, size_t result_size)
{
    if (!result || result_size == 0 || !s_initialized) return ESP_ERR_INVALID_ARG;

    char url[TIANSHU_URL_BUF_SIZE];
    snprintf(url, sizeof(url), "%s/api/v1/gateway/device/call/%d", s_config.server_url, config_id);

    cJSON *root = cJSON_CreateObject();
    if (!root) return ESP_ERR_NO_MEM;

    cJSON_AddStringToObject(root, "device_id", s_config.device_id);
    cJSON_AddStringToObject(root, "secret_key", s_config.secret_key);

    if (params_json) {
        cJSON *params = cJSON_Parse(params_json);
        if (params) cJSON_AddItemToObject(root, "params", params);
    }

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!post_data) return ESP_ERR_NO_MEM;

    char *resp_buf = malloc(4096);
    if (!resp_buf) {
        free(post_data);
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_post_json(url, post_data, resp_buf, 4096, &status);
    free(post_data);

    if (err != ESP_OK || status != 200) {
        free(resp_buf);
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_FAIL;
    cJSON *json = cJSON_Parse(resp_buf);
    if (json) {
        cJSON *data = cJSON_GetObjectItem(json, "data");
        if (data) {
            // 字符串类型直接复制值，避免带引号
            if (cJSON_IsString(data) && data->valuestring) {
                strncpy(result, data->valuestring, result_size - 1);
                result[result_size - 1] = '\0';
                ret = ESP_OK;
            } else {
                // 对象/数组类型序列化为JSON
                char *data_str = cJSON_PrintUnformatted(data);
                if (data_str) {
                    strncpy(result, data_str, result_size - 1);
                    result[result_size - 1] = '\0';
                    free(data_str);
                    ret = ESP_OK;
                }
            }
        }
        cJSON_Delete(json);
    }

    free(resp_buf);
    return ret;
}

esp_err_t tianshu_gateway_direct_call(const ts_api_config_t *config, const char *params_json, char *result, size_t result_size)
{
    if (!config || !result || result_size == 0) return ESP_ERR_INVALID_ARG;
    if (config->proxy_mode != TS_PROXY_DIRECT) return ESP_ERR_INVALID_ARG;

    // base_url已经是完整URL
    const char *url = config->base_url;

    char *resp_buf = malloc(4096);
    if (!resp_buf) return ESP_ERR_NO_MEM;

    int status = 0;
    esp_err_t err;

    // 根据请求方法选择GET或POST
    if (strcmp(config->request_method, "GET") == 0) {
        err = http_get(url, resp_buf, 4096, &status);
    } else {
        // Build request body from template
        char *post_data = NULL;
        if (config->request_body_template[0] && params_json) {
            cJSON *params = cJSON_Parse(params_json);
            if (params) {
                post_data = strdup(config->request_body_template);
                cJSON *item = NULL;
                cJSON_ArrayForEach(item, params) {
                    char placeholder[64];
                    snprintf(placeholder, sizeof(placeholder), "{{%s}}", item->string);
                    char *pos = strstr(post_data, placeholder);
                    if (pos) {
                        const char *val = cJSON_GetStringValue(item);
                        if (val) {
                            char *new_data = malloc(strlen(post_data) + strlen(val) + 1);
                            if (new_data) {
                                size_t prefix_len = pos - post_data;
                                memcpy(new_data, post_data, prefix_len);
                                strcpy(new_data + prefix_len, val);
                                strcpy(new_data + prefix_len + strlen(val), pos + strlen(placeholder));
                                free(post_data);
                                post_data = new_data;
                            }
                        }
                    }
                }
                cJSON_Delete(params);
            }
        }
        err = http_post_json(url, post_data ? post_data : "{}", resp_buf, 4096, &status);
        if (post_data) free(post_data);
    }

    if (err != ESP_OK || status != 200) {
        free(resp_buf);
        return ESP_FAIL;
    }

    strncpy(result, resp_buf, result_size - 1);
    result[result_size - 1] = '\0';
    free(resp_buf);
    return ESP_OK;
}
