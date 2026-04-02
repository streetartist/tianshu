/**
 * TianShu IoT SDK for Arduino
 */

#include "TianShu.h"

// Static instance for callback
TianShu* TianShu::_instance = nullptr;

TianShu::TianShu() {
    _heartbeatInterval = 60;
    _dataInterval = 300;
    _status = TS_DISCONNECTED;
    _cmdCallback = nullptr;
    _lastHeartbeat = 0;
    _reporting = false;
    _wsConnected = false;
    _instance = this;
}

void TianShu::begin(const char* serverUrl, const char* deviceId, const char* secretKey) {
    _serverUrl = serverUrl;
    _deviceId = deviceId;
    _secretKey = secretKey;
    Serial.printf("[TianShu] v%s initialized\n", TIANSHU_VERSION);
    Serial.printf("[TianShu] Device: %s\n", deviceId);
}

void TianShu::setHeartbeatInterval(uint16_t seconds) {
    _heartbeatInterval = seconds;
}

void TianShu::setDataInterval(uint16_t seconds) {
    _dataInterval = seconds;
}

void TianShu::onCommand(TsCommandCallback callback) {
    _cmdCallback = callback;
}

TsStatus TianShu::getStatus() {
    return _status;
}

bool TianShu::connect() {
    _status = TS_CONNECTING;
    if (heartbeat()) {
        _status = TS_CONNECTED;
        Serial.println("[TianShu] Connected");
        return true;
    }
    _status = TS_ERROR;
    Serial.println("[TianShu] Connection failed");
    return false;
}

void TianShu::disconnect() {
    _status = TS_DISCONNECTED;
    Serial.println("[TianShu] Disconnected");
}

bool TianShu::heartbeat() {
    JsonDocument doc;
    doc["device_id"] = _deviceId;
    doc["secret_key"] = _secretKey;

    bool ok = _post("/api/v1/devices/heartbeat", doc);
    if (ok) {
        _lastHeartbeat = millis();
    }
    return ok;
}

bool TianShu::report(const char* name, int value) {
    reportBegin();
    reportAdd(name, value);
    return reportEnd();
}

bool TianShu::report(const char* name, float value) {
    reportBegin();
    reportAdd(name, value);
    return reportEnd();
}

bool TianShu::report(const char* name, bool value) {
    reportBegin();
    reportAdd(name, value);
    return reportEnd();
}

bool TianShu::report(const char* name, const char* value) {
    reportBegin();
    reportAdd(name, value);
    return reportEnd();
}

bool TianShu::reportBegin() {
    _reportDoc.clear();
    _reportDoc["device_id"] = _deviceId;
    _reportDoc["secret_key"] = _secretKey;
    _reportDoc["data"].to<JsonObject>();
    _reporting = true;
    return true;
}

void TianShu::reportAdd(const char* name, int value) {
    if (_reporting) {
        _reportDoc["data"][name] = value;
    }
}

void TianShu::reportAdd(const char* name, float value) {
    if (_reporting) {
        _reportDoc["data"][name] = value;
    }
}

void TianShu::reportAdd(const char* name, bool value) {
    if (_reporting) {
        _reportDoc["data"][name] = value;
    }
}

void TianShu::reportAdd(const char* name, const char* value) {
    if (_reporting) {
        _reportDoc["data"][name] = value;
    }
}

bool TianShu::reportEnd() {
    if (!_reporting) return false;
    _reporting = false;
    return _post("/api/v1/data/report", _reportDoc);
}

void TianShu::loop() {
    // WebSocket loop
    if (_wsConnected) {
        _ws.loop();
    }

    if (_status != TS_CONNECTED) return;

    unsigned long now = millis();
    if (now - _lastHeartbeat >= _heartbeatInterval * 1000UL) {
        heartbeat();
    }
}

bool TianShu::_post(const char* endpoint, JsonDocument& doc) {
    String response;
    return _post(endpoint, doc, response);
}

bool TianShu::_post(const char* endpoint, JsonDocument& doc, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = _serverUrl + endpoint;

    if (_isHttps()) {
        WiFiClientSecure* client = new WiFiClientSecure();
        client->setInsecure();  // Skip certificate verification
        http.begin(*client, url);
    } else {
        http.begin(url);
    }

    http.addHeader("Content-Type", "application/json");

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    if (code == 200) {
        response = http.getString();
    }
    http.end();

    return code == 200;
}

bool TianShu::_get(const char* endpoint, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = _serverUrl + endpoint;

    if (_isHttps()) {
        WiFiClientSecure* client = new WiFiClientSecure();
        client->setInsecure();
        http.begin(*client, url);
    } else {
        http.begin(url);
    }

    int code = http.GET();
    if (code == 200) {
        response = http.getString();
    }
    http.end();

    return code == 200;
}

bool TianShu::_isHttps() {
    return _serverUrl.startsWith("https://");
}

// WebSocket static callback
void TianShu::_wsEventStatic(WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) {
        _instance->_wsEvent(type, payload, length);
    }
}

// 发送设备认证
void TianShu::_sendAuth() {
    JsonDocument auth;
    auth["device_id"] = _deviceId;
    auth["secret_key"] = _secretKey;
    String authStr;
    serializeJson(auth, authStr);
    String msg = "42[\"auth\"," + authStr + "]";
    Serial.printf("[TianShu] Sending auth: %s\n", msg.c_str());
    _ws.sendTXT(msg);
}

// WebSocket event handler
void TianShu::_wsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_CONNECTED:
        Serial.println("[TianShu] WS connected, waiting for Socket.IO handshake...");
        _wsConnected = true;
        break;

    case WStype_DISCONNECTED:
        Serial.println("[TianShu] WS disconnected");
        _wsConnected = false;
        break;

    case WStype_TEXT:
        if (length > 0) {
            String data = (char*)payload;
            Serial.printf("[TianShu] WS recv: %s\n", data.c_str());

            // Socket.IO OPEN packet (0{...})
            if (data[0] == '0') {
                Serial.println("[TianShu] Socket.IO OPEN, sending CONNECT");
                _ws.sendTXT("40");
            }
            // Socket.IO CONNECT confirmation (40{...})
            else if (data.startsWith("40")) {
                Serial.println("[TianShu] Socket.IO CONNECT confirmed, sending auth");
                _sendAuth();
            }
            // Socket.IO PING (2) - respond with PONG (3)
            else if (data[0] == '2') {
                _ws.sendTXT("3");
            }
            // Socket.IO MESSAGE - command (42["command",...])
            else if (data.startsWith("42[\"command\"") && _cmdCallback) {
                int idx = data.indexOf('{');
                if (idx > 0) {
                    String json = data.substring(idx);
                    json = json.substring(0, json.lastIndexOf('}') + 1);
                    JsonDocument doc;
                    if (!deserializeJson(doc, json)) {
                        const char* cmd = doc["command"];
                        JsonObject params = doc["params"];
                        if (cmd) {
                            _cmdCallback(cmd, params);
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

bool TianShu::wsConnect() {
    // Parse server URL
    String url = _serverUrl;
    url.replace("http://", "");
    url.replace("https://", "");

    int portIdx = url.indexOf(':');
    String host;
    uint16_t port = 80;

    if (portIdx > 0) {
        host = url.substring(0, portIdx);
        port = url.substring(portIdx + 1).toInt();
    } else {
        host = url;
    }

    Serial.printf("[TianShu] WS connecting to %s:%d\n", host.c_str(), port);

    _ws.begin(host, port, "/socket.io/?EIO=4&transport=websocket");
    _ws.onEvent(_wsEventStatic);
    _ws.setReconnectInterval(5000);

    return true;
}

void TianShu::wsDisconnect() {
    _ws.disconnect();
    _wsConnected = false;
    Serial.println("[TianShu] WS disconnected");
}

bool TianShu::wsIsConnected() {
    return _wsConnected;
}

void TianShu::wsSendCmdResult(const char* cmdId, bool success, const char* result) {
    if (!_wsConnected) return;

    JsonDocument doc;
    doc["device_id"] = _deviceId;
    doc["cmd_id"] = cmdId;
    doc["success"] = success;
    if (result) {
        doc["result"] = result;
    }

    String dataStr;
    serializeJson(doc, dataStr);
    String msg = "42[\"cmd_result\"," + dataStr + "]";
    _ws.sendTXT(msg);
}

// Get device config
bool TianShu::getConfig(String& config) {
    String endpoint = "/api/v1/devices/" + _deviceId + "/config?secret_key=" + _secretKey;
    return _get(endpoint.c_str(), config);
}

// Get API configurations
int TianShu::gatewayGetConfigs(TsApiConfig* configs, int maxCount) {
    String endpoint = "/api/v1/devices/" + _deviceId + "/api-configs?secret_key=" + _secretKey;
    String response;

    if (!_get(endpoint.c_str(), response)) {
        return 0;
    }

    JsonDocument doc;
    if (deserializeJson(doc, response)) {
        return 0;
    }

    JsonArray arr = doc["configs"].as<JsonArray>();
    int count = 0;

    for (JsonObject obj : arr) {
        if (count >= maxCount) break;

        configs[count].id = obj["id"] | 0;
        configs[count].name = obj["name"] | "";
        configs[count].apiType = (TsApiType)(obj["api_type"] | 0);
        configs[count].proxyMode = (TsProxyMode)(obj["proxy_mode"] | 0);
        configs[count].timeout = obj["timeout"] | 30000;
        configs[count].baseUrl = obj["base_url"] | "";
        configs[count].apiKey = obj["api_key"] | "";
        configs[count].requestMethod = obj["request_method"] | "POST";
        configs[count].requestBodyTemplate = obj["request_body_template"] | "";
        configs[count].responseExtract = obj["response_extract"] | "";
        count++;
    }

    return count;
}

// Call API via server proxy
bool TianShu::gatewayCall(int configId, const char* paramsJson, String& result) {
    JsonDocument doc;
    doc["device_id"] = _deviceId;
    doc["secret_key"] = _secretKey;
    doc["config_id"] = configId;
    doc["params"] = paramsJson ? paramsJson : "{}";

    return _post("/api/v1/gateway/call", doc, result);
}

// Direct call to external API
bool TianShu::gatewayDirectCall(const TsApiConfig& config, const char* paramsJson, String& result) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = config.baseUrl;

    if (url.startsWith("https://")) {
        WiFiClientSecure* client = new WiFiClientSecure();
        client->setInsecure();
        http.begin(*client, url);
    } else {
        http.begin(url);
    }

    http.addHeader("Content-Type", "application/json");
    if (config.apiKey.length() > 0) {
        http.addHeader("Authorization", "Bearer " + config.apiKey);
    }
    http.setTimeout(config.timeout);

    // Build request body from template
    String body = config.requestBodyTemplate;
    if (paramsJson) {
        body.replace("{{params}}", paramsJson);
    }

    int code;
    if (config.requestMethod == "GET") {
        code = http.GET();
    } else {
        code = http.POST(body);
    }

    if (code == 200) {
        result = http.getString();
    }
    http.end();

    return code == 200;
}
