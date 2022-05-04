//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ble.h"
#include "button.h"
#include "debug_heartbeat.h"
#include "envi_config.h"
#include "lcd.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sht21.h"
#include <assert.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "ENVI_SENSOR_MAIN"
#include "iferr.h"

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

typedef struct
{
    float temperature;
    float humidity;
} sensor_reading_t;

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void create_task(TaskFunction_t fn, const char *const name, UBaseType_t priority);

static void button_isr_handler(void *param);

static void task_read_sensor(void *param);

static void task_update_ble(void *param);

static void task_update_lcd_ring_buffer(void *param);

static void task_render_lcd_view(void *param);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

// pass sensor readings from the sensor to the BLE peripheral
static QueueHandle_t binqueue_ble = NULL;

// pass sensor readings from the sensor to the onboard monitor
static QueueHandle_t binqueue_lcd = NULL;

// binsemaphore_lcd_render informs a task when the lcd_view has been updated
static SemaphoreHandle_t binsemaphore_lcd_render = NULL;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

void app_main(void)
{
    ESP_LOGI(ESP_LOG_TAG, "initialize peripherals and tasks");
    binqueue_ble = xQueueCreate(1, sizeof(sensor_reading_t));
    binqueue_lcd = xQueueCreate(1, sizeof(sensor_reading_t));
    binsemaphore_lcd_render = xSemaphoreCreateBinary();

    ESP_ERROR_CHECK(ble_init());
    ESP_ERROR_CHECK(button_init(button_isr_handler));
    ESP_ERROR_CHECK(debug_heartbeat_init(HEARTBEAT_PIN));
    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(sht21_init(0, SENSOR_SDA_PIN, SENSOR_SCL_PIN, sht21_i2c_speed_standard));

    create_task(task_read_sensor, "task_read_sensor", TASK_PRIORITY_READ_SENSOR);
    create_task(task_update_ble, "task_update_ble", TASK_PRIORITY_UPDATE_BLE);
    create_task(task_update_lcd_ring_buffer, "task_update_lcd_ring_buffer", TASK_PRIORITY_UPDATE_LCD_RING_BUFFER);
    create_task(task_render_lcd_view, "task_render_lcd_view", TASK_PRIORITY_RENDER_LCD_VIEW);

    vTaskDelete(NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void create_task(TaskFunction_t fn, const char *const name, UBaseType_t priority)
{
    TaskHandle_t task_handle = NULL;
    xTaskCreate(fn, name, TASK_STACK_DEPTH, NULL, priority, &task_handle);
    assert(task_handle);
}

static void button_isr_handler(void *param)
{
    button_debounce();
    lcd_select_next_view();
    xSemaphoreGiveFromISR(binsemaphore_lcd_render, NULL);
}

static void task_read_sensor(void *param)
{
    const TickType_t frequency = CONFIG_READ_SENSOR_FREQUENCY_MS / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (1)
    {
        ESP_LOGI(ESP_LOG_TAG, "read sensor");
        esp_err_t err;
        float temperature_reading;
        float humidity_reading;
        if ((err = sht21_get_temperature(&temperature_reading)) != ESP_OK)
        {
            IFERR_LOG(err, "could not read temperature");
            continue;
        }
        if ((err = sht21_get_humidity(&humidity_reading)) != ESP_OK)
        {
            IFERR_LOG(err, "could not read humidity");
            continue;
        }
        if (humidity_reading < 0)
            humidity_reading = 0;
        if (humidity_reading > 100)
            humidity_reading = 100;

        sensor_reading_t reading = {.temperature = temperature_reading, .humidity = humidity_reading};
        if (xQueueSend(binqueue_ble, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from BLE peripheral, overwriting with new value");
            assert(xQueueOverwrite(binqueue_ble, (void *)&reading));
        }
        if (xQueueSend(binqueue_lcd, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from monitor, overwriting with new value");
            assert(xQueueOverwrite(binqueue_lcd, (void *)&reading));
        }
        if (xSemaphoreGive(binsemaphore_lcd_render) != pdTRUE)
        {
            ESP_LOGW(ESP_LOG_TAG, "failed to give binsemaphore_lcd_render");
        }
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

static void task_update_ble(void *param)
{
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(binqueue_ble, &reading, portMAX_DELAY))
        {
            ESP_LOGI(ESP_LOG_TAG, "update ble charact , temp: %f humid: %f", reading.temperature, reading.humidity);
            IFERR_LOG(ble_write_temperature(reading.temperature), "failed to write temperature");
            IFERR_LOG(ble_write_humidity(reading.humidity), "failed to write humidity");
        }
    }
}

static void task_update_lcd_ring_buffer(void *param)
{
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(binqueue_lcd, &reading, portMAX_DELAY))
        {
            ESP_LOGI(ESP_LOG_TAG, "update ring-buffers, temp: %f humid: %f", reading.temperature, reading.humidity);
            lcd_store_temperature(reading.temperature);
            lcd_store_humidity(reading.humidity);
        }
    }
}

static void task_render_lcd_view(void *param)
{
    while (1)
    {
        while (!xSemaphoreTake(binsemaphore_lcd_render, portMAX_DELAY))
        {
        }
        ESP_LOGI(ESP_LOG_TAG, "render lcd");
        lcd_render();
    }
}
