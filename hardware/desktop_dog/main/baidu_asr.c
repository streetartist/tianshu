#include "baidu_asr.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

static const char *TAG = "BAIDU_ASR";

#define API_KEY    "YOUR_BAIDU_API_KEY"
#define SECRET_KEY "YOUR_BAIDU_SECRET_KEY"
#define CUID       "esp32s3_desktop_dog"

#define WEB_SERVER "vop.baidu.com"
#define WEB_PORT   "80"

char s_access_token[512] = {0};

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

esp_err_t baidu_asr_get_token(char *token, size_t token_size)
{
    char url[256];
    snprintf(url, sizeof(url),
        "https://aip.baidubce.com/oauth/2.0/token?"
        "grant_type=client_credentials&client_id=%s&client_secret=%s",
        API_KEY, SECRET_KEY);

    char resp_buf[1024] = {0};
    http_response_t resp = {.data = resp_buf, .len = 0, .cap = sizeof(resp_buf)};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        // 直接搜索access_token，不需要完整JSON解析
        char *tk_start = strstr(resp.data, "\"access_token\":\"");
        if (tk_start) {
            tk_start += 16;  // 跳过 "access_token":"
            char *tk_end = strchr(tk_start, '"');
            if (tk_end) {
                size_t tk_len = tk_end - tk_start;
                if (tk_len < token_size && tk_len < sizeof(s_access_token)) {
                    strncpy(token, tk_start, tk_len);
                    token[tk_len] = '\0';
                    strncpy(s_access_token, tk_start, tk_len);
                    s_access_token[tk_len] = '\0';
                    ESP_LOGI(TAG, "Got access token, len=%d", tk_len);
                }
            }
        } else {
            ESP_LOGE(TAG, "No access_token found");
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

// 解析HTTP响应，提取JSON body（支持chunked编码）
static int parse_http_response(char *data, int len, char *result, size_t result_size)
{
    // 找到空行（header和body的分隔）
    char *body = strstr(data, "\r\n\r\n");
    if (!body) {
        ESP_LOGE(TAG, "No body found");
        return -1;
    }
    body += 4;

    // 检查是否是chunked编码（跳过chunk大小行）
    char *json_start = strchr(body, '{');
    if (!json_start) {
        ESP_LOGE(TAG, "No JSON found");
        return -1;
    }

    // 解析JSON
    cJSON *json = cJSON_Parse(json_start);
    if (!json) {
        ESP_LOGE(TAG, "JSON parse failed: %s", json_start);
        return -1;
    }

    int ret = -1;
    cJSON *err_no = cJSON_GetObjectItem(json, "err_no");
    if (err_no) {
        ret = err_no->valueint;
        if (ret == 0) {
            cJSON *res = cJSON_GetObjectItem(json, "result");
            if (res && cJSON_IsArray(res)) {
                cJSON *first = cJSON_GetArrayItem(res, 0);
                if (first && first->valuestring) {
                    strncpy(result, first->valuestring, result_size - 1);
                }
            }
        } else {
            ESP_LOGE(TAG, "ASR error: %d", ret);
        }
    }

    cJSON_Delete(json);
    return ret;
}

esp_err_t baidu_asr_recognize(const int16_t *audio, size_t samples, char *result, size_t result_size)
{
    size_t audio_len = samples * sizeof(int16_t);
    ESP_LOGI(TAG, "Audio len: %d, Token len: %d", audio_len, strlen(s_access_token));

    // DNS解析
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;

    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed");
        return ESP_FAIL;
    }

    // 创建socket
    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket create failed");
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    // 连接服务器
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "Socket connect failed");
        close(sock);
        freeaddrinfo(res);
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    // 构建HTTP请求头（使用完整URL格式，和参考代码一致）
    char header[768];
    int header_len = snprintf(header, sizeof(header),
        "POST http://vop.baidu.com/server_api?cuid=%s&token=%s HTTP/1.1\r\n"
        "Accept-Encoding: identity\r\n"
        "Host: vop.baidu.com\r\n"
        "User-Agent: esp32s3\r\n"
        "Content-Type: audio/pcm; rate=16000\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n",
        CUID, s_access_token);

    // 发送请求头
    if (write(sock, header, header_len) < 0) {
        ESP_LOGE(TAG, "Header send failed");
        close(sock);
        return ESP_FAIL;
    }

    // 分块发送音频数据
    const uint8_t *audio_ptr = (const uint8_t *)audio;
    size_t remaining = audio_len;
    size_t chunk_size = 1000;  // 每块1000字节
    char chunk_header[16];

    while (remaining > 0) {
        size_t send_size = (remaining > chunk_size) ? chunk_size : remaining;

        // 发送chunk大小（十六进制）
        int ch_len = snprintf(chunk_header, sizeof(chunk_header), "%X\r\n", send_size);
        write(sock, chunk_header, ch_len);

        // 发送chunk数据
        write(sock, audio_ptr, send_size);

        // 发送chunk结束符
        write(sock, "\r\n", 2);

        audio_ptr += send_size;
        remaining -= send_size;
    }

    // 发送结束标志
    write(sock, "0\r\n\r\n", 5);

    // 接收响应
    char recv_buf[1024] = {0};
    int recv_len = read(sock, recv_buf, sizeof(recv_buf) - 1);
    close(sock);

    if (recv_len <= 0) {
        ESP_LOGE(TAG, "Recv failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Response: %s", recv_buf);

    // 解析响应
    if (parse_http_response(recv_buf, recv_len, result, result_size) == 0) {
        return ESP_OK;
    }

    return ESP_FAIL;
}
