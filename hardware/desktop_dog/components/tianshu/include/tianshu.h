/**
 * TianShu IoT SDK for ESP-IDF
 * 天枢物联网平台 ESP-IDF 开发库
 */

#ifndef TIANSHU_H
#define TIANSHU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Version
#define TIANSHU_VERSION "1.2.0"

// Max API configs
#define TS_MAX_API_CONFIGS 16

// Data types
typedef enum {
    TS_TYPE_INT = 0,
    TS_TYPE_FLOAT,
    TS_TYPE_BOOL,
    TS_TYPE_STRING,
    TS_TYPE_JSON
} ts_data_type_t;

// Connection status
typedef enum {
    TS_STATUS_DISCONNECTED = 0,
    TS_STATUS_CONNECTING,
    TS_STATUS_CONNECTED,
    TS_STATUS_ERROR
} ts_status_t;

// API types
typedef enum {
    TS_API_TYPE_AI_MODEL = 0,
    TS_API_TYPE_NORMAL_API,
    TS_API_TYPE_PYTHON_HANDLER
} ts_api_type_t;

// Proxy modes
typedef enum {
    TS_PROXY_SERVER = 0,
    TS_PROXY_DIRECT
} ts_proxy_mode_t;

// Data point structure
typedef struct {
    const char *name;
    ts_data_type_t type;
    union {
        int32_t i;
        float f;
        bool b;
        const char *s;
    } value;
} ts_datapoint_t;

// Command callback
typedef void (*ts_command_cb_t)(const char *cmd, const char *params);

// API configuration structure
typedef struct {
    int id;
    char name[64];
    ts_api_type_t api_type;
    ts_proxy_mode_t proxy_mode;
    int timeout;
    char base_url[256];
    char api_key[256];
    char request_method[8];
    char request_body_template[1024];
    char response_extract[128];
} ts_api_config_t;

// Configuration
typedef struct {
    const char *server_url;
    const char *device_id;
    const char *secret_key;
    uint16_t heartbeat_interval;
    uint16_t data_interval;
    ts_command_cb_t command_callback;
} ts_config_t;

// Initialize SDK
esp_err_t tianshu_init(const ts_config_t *config);

// Connect to server
esp_err_t tianshu_connect(void);

// Disconnect
void tianshu_disconnect(void);

// Get status
ts_status_t tianshu_get_status(void);

// Report single data point
esp_err_t tianshu_report(const char *name, ts_data_type_t type, void *value);

// Report multiple data points
esp_err_t tianshu_report_batch(ts_datapoint_t *points, size_t count);

// Send heartbeat
esp_err_t tianshu_heartbeat(void);

// Get device config from server
esp_err_t tianshu_get_config(char *buf, size_t buf_size);

// WebSocket functions
esp_err_t tianshu_ws_connect(void);
void tianshu_ws_disconnect(void);
bool tianshu_ws_is_connected(void);
esp_err_t tianshu_ws_send_cmd_result(const char *cmd_id, bool success, const char *result);

// ============ API Gateway Functions ============

// Get API configurations from server
int tianshu_gateway_get_configs(ts_api_config_t *configs, int max_count);

// Call API via server proxy (works for all types including python_handler)
esp_err_t tianshu_gateway_call(int config_id, const char *params_json, char *result, size_t result_size);

// Direct call to external API (for direct proxy mode only)
esp_err_t tianshu_gateway_direct_call(const ts_api_config_t *config, const char *params_json, char *result, size_t result_size);

#ifdef __cplusplus
}
#endif

#endif // TIANSHU_H
