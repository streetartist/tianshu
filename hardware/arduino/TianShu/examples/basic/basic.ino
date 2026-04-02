/**
 * TianShu Basic Example
 * 天枢物联网平台基础示例
 */

#include <WiFi.h>
#include <TianShu.h>

// WiFi配置
const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";

// 天枢平台配置
const char* SERVER_URL = "http://your-server:5000";
const char* DEVICE_ID = "your-device-id";
const char* SECRET_KEY = "your-secret-key";

TianShu ts;

unsigned long lastReport = 0;
const unsigned long REPORT_INTERVAL = 60000;

void setup() {
    Serial.begin(115200);

    // 连接WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // 初始化天枢SDK
    ts.begin(SERVER_URL, DEVICE_ID, SECRET_KEY);
    ts.setHeartbeatInterval(60);

    // 连接服务器
    ts.connect();
}

void loop() {
    ts.loop();

    // 定时上报数据
    if (millis() - lastReport >= REPORT_INTERVAL) {
        lastReport = millis();

        // 上报多个数据点
        ts.reportBegin();
        ts.reportAdd("temperature", 25.5);
        ts.reportAdd("humidity", 60);
        ts.reportEnd();

        Serial.println("Data reported");
    }
}
