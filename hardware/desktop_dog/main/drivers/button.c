/**
 * @file button.c
 * @brief Button driver with debounce and long press detection
 */

#include "button.h"
#include "app_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

/* Button GPIO pins */
static const gpio_num_t s_button_gpios[BUTTON_COUNT] = {
    BUTTON_1_GPIO,
    BUTTON_2_GPIO,
    BUTTON_3_GPIO,
    BUTTON_4_GPIO
};

/* Button state tracking */
typedef struct {
    uint32_t press_time;    /* Time when button was pressed */
    bool pressed;           /* Current pressed state */
    bool handled;           /* Whether this press has been handled */
} button_state_t;

static button_state_t s_button_states[BUTTON_COUNT];
static button_event_cb_t s_callback = NULL;
static TaskHandle_t s_task_handle = NULL;
static QueueHandle_t s_event_queue = NULL;
static volatile bool s_running = false;

/* Button event for queue */
typedef struct {
    uint8_t button_id;
    bool pressed;
    uint32_t time_ms;
} button_event_t;

/**
 * @brief GPIO ISR handler
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint8_t button_id = (uint8_t)(uintptr_t)arg;
    button_event_t evt = {
        .button_id = button_id,
        .pressed = (gpio_get_level(s_button_gpios[button_id]) == 0), /* Active low */
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000)
    };
    xQueueSendFromISR(s_event_queue, &evt, NULL);
}

/**
 * @brief Button processing task
 */
static void button_task(void *arg)
{
    button_event_t evt;

    ESP_LOGI(TAG, "Button task started");

    while (s_running) {
        /* Check for button events with timeout */
        if (xQueueReceive(s_event_queue, &evt, pdMS_TO_TICKS(50)) == pdTRUE) {
            button_state_t *state = &s_button_states[evt.button_id];

            if (evt.pressed && !state->pressed) {
                /* Button pressed - start timing */
                state->press_time = evt.time_ms;
                state->pressed = true;
                state->handled = false;
                ESP_LOGD(TAG, "Button %d pressed at %lu", evt.button_id, evt.time_ms);
            }
            else if (!evt.pressed && state->pressed) {
                /* Button released */
                uint32_t duration = evt.time_ms - state->press_time;
                state->pressed = false;

                /* Debounce check */
                if (duration >= BUTTON_DEBOUNCE_MS && !state->handled) {
                    bool long_press = (duration >= BUTTON_LONG_PRESS_MS);
                    ESP_LOGI(TAG, "Button %d %s (duration: %lums)",
                             evt.button_id,
                             long_press ? "long press" : "short press",
                             duration);

                    if (s_callback) {
                        s_callback(evt.button_id, long_press);
                    }
                }
            }
        }

        /* Check for long press while button is held */
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        for (int i = 0; i < BUTTON_COUNT; i++) {
            button_state_t *state = &s_button_states[i];
            if (state->pressed && !state->handled) {
                uint32_t duration = now - state->press_time;
                if (duration >= BUTTON_LONG_PRESS_MS) {
                    /* Trigger long press callback immediately */
                    ESP_LOGI(TAG, "Button %d long press triggered", i);
                    state->handled = true;
                    if (s_callback) {
                        s_callback(i, true);
                    }
                }
            }
        }
    }

    ESP_LOGI(TAG, "Button task ended");
    vTaskDelete(NULL);
}

esp_err_t button_init(button_event_cb_t callback)
{
    if (s_running) {
        return ESP_ERR_INVALID_STATE;
    }

    s_callback = callback;

    /* Create event queue */
    s_event_queue = xQueueCreate(16, sizeof(button_event_t));
    if (!s_event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize button states */
    for (int i = 0; i < BUTTON_COUNT; i++) {
        s_button_states[i].pressed = false;
        s_button_states[i].handled = false;
        s_button_states[i].press_time = 0;
    }

    /* Configure GPIO pins */
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    for (int i = 0; i < BUTTON_COUNT; i++) {
        io_conf.pin_bit_mask |= (1ULL << s_button_gpios[i]);
    }

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", err);
        vQueueDelete(s_event_queue);
        return err;
    }

    /* Install GPIO ISR service */
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means already installed, which is OK */
        ESP_LOGE(TAG, "GPIO ISR service install failed: %d", err);
        vQueueDelete(s_event_queue);
        return err;
    }

    /* Attach ISR handlers */
    for (int i = 0; i < BUTTON_COUNT; i++) {
        err = gpio_isr_handler_add(s_button_gpios[i], gpio_isr_handler, (void *)(uintptr_t)i);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ISR for button %d: %d", i, err);
        }
    }

    /* Start button task */
    s_running = true;
    BaseType_t ret = xTaskCreate(
        button_task,
        "button",
        2048,
        NULL,
        5,
        &s_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        s_running = false;
        for (int i = 0; i < BUTTON_COUNT; i++) {
            gpio_isr_handler_remove(s_button_gpios[i]);
        }
        vQueueDelete(s_event_queue);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Button driver initialized (GPIOs: %d, %d, %d, %d)",
             BUTTON_1_GPIO, BUTTON_2_GPIO, BUTTON_3_GPIO, BUTTON_4_GPIO);

    return ESP_OK;
}

void button_deinit(void)
{
    if (!s_running) {
        return;
    }

    s_running = false;

    /* Wait for task to end */
    if (s_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_task_handle = NULL;
    }

    /* Remove ISR handlers */
    for (int i = 0; i < BUTTON_COUNT; i++) {
        gpio_isr_handler_remove(s_button_gpios[i]);
    }

    /* Delete queue */
    if (s_event_queue) {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
    }

    s_callback = NULL;

    ESP_LOGI(TAG, "Button driver deinitialized");
}
