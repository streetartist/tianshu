/**
 * TianShu IoT SDK for Arduino
 * 天枢物联网平台 Arduino 开发库
 */

#ifndef TIANSHU_H
#define TIANSHU_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define TIANSHU_VERSION "1.2.0"
#define TS_MAX_API_CONFIGS 16

// Data types
enum TsDataType {
    TS_INT = 0,
    TS_FLOAT,
    TS_BOOL,
    TS_STRING,
    TS_JSON
};

// Status
enum TsStatus {
    TS_DISCONNECTED = 0,
    TS_CONNECTING,
    TS_CONNECTED,
    TS_ERROR
};

// API types
enum TsApiType {
    TS_API_AI_MODEL = 0,
    TS_API_NORMAL,
    TS_API_PYTHON_HANDLER
};

// Proxy modes
enum TsProxyMode {
    TS_PROXY_SERVER = 0,
    TS_PROXY_DIRECT
};

// API configuration
struct TsApiConfig {
    int id;
    String name;
    TsApiType apiType;
    TsProxyMode proxyMode;
    int timeout;
    String baseUrl;
    String apiKey;
    String requestMethod;
    String requestBodyTemplate;
    String responseExtract;
};

// Command callback
typedef void (*TsCommandCallback)(const char* cmd, JsonObject& params);

class TianShu {
public:
    TianShu();

    // Initialize
    void begin(const char* serverUrl, const char* deviceId, const char* secretKey);

    // Set intervals
    void setHeartbeatInterval(uint16_t seconds);
    void setDataInterval(uint16_t seconds);

    // Set command callback
    void onCommand(TsCommandCallback callback);

    // Connect
    bool connect();
    void disconnect();

    // Get status
    TsStatus getStatus();

    // Heartbeat
    bool heartbeat();

    // Report data
    bool report(const char* name, int value);
    bool report(const char* name, float value);
    bool report(const char* name, bool value);
    bool report(const char* name, const char* value);

    // Report multiple
    bool reportBegin();
    void reportAdd(const char* name, int value);
    void reportAdd(const char* name, float value);
    void reportAdd(const char* name, bool value);
    void reportAdd(const char* name, const char* value);
    bool reportEnd();

    // Loop (call in loop())
    void loop();

    // WebSocket functions
    bool wsConnect();
    void wsDisconnect();
    bool wsIsConnected();
    void wsSendCmdResult(const char* cmdId, bool success, const char* result = nullptr);

    // Get device config
    bool getConfig(String& config);

    // API Gateway functions
    int gatewayGetConfigs(TsApiConfig* configs, int maxCount);
    bool gatewayCall(int configId, const char* paramsJson, String& result);
    bool gatewayDirectCall(const TsApiConfig& config, const char* paramsJson, String& result);

private:
    String _serverUrl;
    String _deviceId;
    String _secretKey;
    uint16_t _heartbeatInterval;
    uint16_t _dataInterval;
    TsStatus _status;
    TsCommandCallback _cmdCallback;

    unsigned long _lastHeartbeat;
    JsonDocument _reportDoc;
    bool _reporting;

    // WebSocket
    WebSocketsClient _ws;
    bool _wsConnected;
    void _wsEvent(WStype_t type, uint8_t* payload, size_t length);
    void _sendAuth();
    static void _wsEventStatic(WStype_t type, uint8_t* payload, size_t length);
    static TianShu* _instance;

    bool _post(const char* endpoint, JsonDocument& doc);
    bool _post(const char* endpoint, JsonDocument& doc, String& response);
    bool _get(const char* endpoint, String& response);
    bool _isHttps();
};

#endif
