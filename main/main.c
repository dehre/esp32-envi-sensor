//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ble_handler.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define MAIN_TAG "MAIN"

#define TASK_STACK_DEPTH 2048

#define MAIN_TASK_PRIORITY 1                          // for reference
#define MAX_TASK_PRIORITY (configMAX_PRIORITIES - 1U) // for reference
#define READ_SENSOR_TASK_PRIORITY 2
#define UPDATE_MONITOR_TASK_PRIORITY 2

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

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

// Pass readings from the sensor to the onboard monitor
static QueueHandle_t mailbox_monitor = NULL;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

void app_main(void)
{
    ESP_ERROR_CHECK(ble_handler_init());

    mailbox_monitor = xQueueCreate(1, sizeof(sensor_reading_t));

    TaskHandle_t periodic_task_handle = NULL;
    xTaskCreate(&read_sensor_task, "read_sensor_task", TASK_STACK_DEPTH, NULL, READ_SENSOR_TASK_PRIORITY,
                &periodic_task_handle);
    configASSERT(periodic_task_handle);

    TaskHandle_t update_monitor_task_handle = NULL;
    xTaskCreate(&update_monitor_task, "update_monitor_task", TASK_STACK_DEPTH, NULL, UPDATE_MONITOR_TASK_PRIORITY,
                &update_monitor_task_handle);
    configASSERT(update_monitor_task_handle);

    vTaskDelete(NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void read_sensor_task(void *param)
{
    (void)param;
    const TickType_t frequency = 1000 / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();
    for (int i = -10;; i++)
    {
        vTaskDelayUntil(&lastWakeTime, frequency);

        sensor_reading_t reading = {.temperature = (float)i, .humidity = (i / (float)2.0)};
        if (xQueueSend(mailbox_monitor, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(MAIN_TAG, "Last sensor reading not received from monitor, overwriting with new value");
            configASSERT(xQueueOverwrite(mailbox_monitor, (void *)&reading));
        }
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
            printf("MONITOR -- temperature: %f humidity: %f\n", reading.temperature, reading.humidity);
        }
    }
}
