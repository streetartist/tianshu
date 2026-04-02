#include "dht11.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DHT11";
static int dht_gpio = -1;
static float last_temperature = 0;
static float last_humidity = 0;

esp_err_t dht11_init(int gpio_num)
{
    dht_gpio = gpio_num;
    gpio_reset_pin(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_level(gpio_num, 1);
    ESP_LOGI(TAG, "DHT11 initialized on GPIO %d", gpio_num);
    return ESP_OK;
}

static int wait_for_level(int level, int timeout_us)
{
    int elapsed = 0;
    while (gpio_get_level(dht_gpio) != level) {
        if (elapsed >= timeout_us) return -1;
        esp_rom_delay_us(1);
        elapsed++;
    }
    return elapsed;
}

esp_err_t dht11_read(float *temperature, float *humidity)
{
    if (dht_gpio < 0) return ESP_ERR_INVALID_STATE;

    uint8_t data[5] = {0};

    // 禁用中断以确保时序准确
    portDISABLE_INTERRUPTS();

    // 发送起始信号：拉低至少18ms
    gpio_set_level(dht_gpio, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(dht_gpio, 1);
    esp_rom_delay_us(30);

    // 等待DHT11响应：先拉低80us，再拉高80us
    if (wait_for_level(0, 100) < 0) {
        portENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_level(1, 100) < 0) {
        portENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_level(0, 100) < 0) {
        portENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }

    // 读取40位数据
    for (int i = 0; i < 40; i++) {
        // 等待低电平结束（50us）
        if (wait_for_level(1, 100) < 0) {
            portENABLE_INTERRUPTS();
            return ESP_ERR_TIMEOUT;
        }
        // 测量高电平持续时间：26-28us为0，70us为1
        int high_time = wait_for_level(0, 100);
        if (high_time < 0) {
            portENABLE_INTERRUPTS();
            return ESP_ERR_TIMEOUT;
        }
        // 高电平超过40us认为是1
        if (high_time > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    portENABLE_INTERRUPTS();

    // 校验
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "Checksum error: %d != %d", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    *humidity = data[0] + data[1] * 0.1f;
    *temperature = data[2] + data[3] * 0.1f;

    // 保存最新数据
    last_temperature = *temperature;
    last_humidity = *humidity;

    return ESP_OK;
}

float dht11_get_temperature(void)
{
    return last_temperature;
}

float dht11_get_humidity(void)
{
    return last_humidity;
}
