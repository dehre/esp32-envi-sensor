//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ble_manager.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nokia_5110_lcd.h"
#include "sht21.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "MAIN"
#include "iferr.h"

#define TASK_STACK_DEPTH 2048

#define MAIN_TASK_PRIORITY 1                          // for reference
#define MAX_TASK_PRIORITY (configMAX_PRIORITIES - 1U) // for reference
#define READ_SENSOR_TASK_PRIORITY 2
#define UPDATE_MONITOR_TASK_PRIORITY 2
#define UPDATE_BLE_TASK_PRIORITY 2

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

static void read_sensor_task(void *param);

static void update_monitor_task(void *param);

static void update_ble_task(void *param);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

// Pass sensor readings from the sensor to the onboard monitor
static QueueHandle_t mailbox_monitor = NULL;

// Pass sensor readings from the sensor to the BLE peripheral
static QueueHandle_t mailbox_ble = NULL;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

void app_main(void)
{
    ESP_ERROR_CHECK(sht21_init(0, GPIO_NUM_32, GPIO_NUM_33, sht21_i2c_speed_standard));
    ESP_ERROR_CHECK(ble_manager_init());
    nokia_5110_lcd_init();

    mailbox_monitor = xQueueCreate(1, sizeof(sensor_reading_t));
    mailbox_ble = xQueueCreate(1, sizeof(sensor_reading_t));

    TaskHandle_t periodic_task_handle = NULL;
    xTaskCreate(&read_sensor_task, "read_sensor_task", TASK_STACK_DEPTH, NULL, READ_SENSOR_TASK_PRIORITY,
                &periodic_task_handle);
    configASSERT(periodic_task_handle);

    TaskHandle_t update_monitor_task_handle = NULL;
    xTaskCreate(&update_monitor_task, "update_monitor_task", TASK_STACK_DEPTH, NULL, UPDATE_MONITOR_TASK_PRIORITY,
                &update_monitor_task_handle);
    configASSERT(update_monitor_task_handle);

    TaskHandle_t update_ble_task_handle = NULL;
    xTaskCreate(&update_ble_task, "update_ble_task", TASK_STACK_DEPTH, NULL, UPDATE_BLE_TASK_PRIORITY,
                &update_ble_task_handle);
    configASSERT(update_ble_task_handle);

    vTaskDelete(NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void read_sensor_task(void *param)
{
    (void)param;
    const TickType_t frequency = 5000 / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (1)
    {
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
        if (xQueueSend(mailbox_monitor, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from monitor, overwriting with new value");
            configASSERT(xQueueOverwrite(mailbox_monitor, (void *)&reading));
        }
        if (xQueueSend(mailbox_ble, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from BLE peripheral, overwriting with new value");
            configASSERT(xQueueOverwrite(mailbox_ble, (void *)&reading));
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

static void update_monitor_task(void *param)
{
    (void)param;
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(mailbox_monitor, &reading, portMAX_DELAY))
        {
            nokia_5110_lcd_write_temperature(reading.temperature);
            nokia_5110_lcd_write_humidity(reading.humidity);
        }
    }
}

static void update_ble_task(void *param)
{
    (void)param;
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(mailbox_ble, &reading, portMAX_DELAY))
        {
            IFERR_LOG(ble_manager_write_temperature(reading.temperature), "failed to write temperature");
            IFERR_LOG(ble_manager_write_humidity(reading.humidity), "failed to write humidity");
        }
    }
}
