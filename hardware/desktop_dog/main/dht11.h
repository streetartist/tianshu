#ifndef DHT11_H
#define DHT11_H

#include "esp_err.h"

esp_err_t dht11_init(int gpio_num);
esp_err_t dht11_read(float *temperature, float *humidity);

// 获取最新的温湿度数据
float dht11_get_temperature(void);
float dht11_get_humidity(void);

#endif // DHT11_H
