#ifndef DATA_REPORT_H
#define DATA_REPORT_H

#include "esp_err.h"

// 服务器配置 - 请填写你的服务器地址
#define REPORT_SERVER_URL   ""  // 例如: "http://192.168.1.100:5000/api/v1/data/report"
#define REPORT_DEVICE_ID    ""  // 设备ID
#define REPORT_DEVICE_KEY   ""  // 设备密钥

// 上报间隔（秒）
#define REPORT_INTERVAL_SEC  60

// 初始化数据上报模块
esp_err_t data_report_init(void);

// 上报温湿度数据
esp_err_t data_report_dht11(float temperature, float humidity);

// 启动定时上报任务
void data_report_start_task(void);

#endif // DATA_REPORT_H
