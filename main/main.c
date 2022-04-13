#include "ble_handler.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define MAIN_TAG "MAIN"

#define TASK_STACK_DEPTH 2048

#define MAIN_TASK_PRIORITY 1                          // for reference
#define MAX_TASK_PRIORITY (configMAX_PRIORITIES - 1U) // for reference
#define PERIODIC_TASK_PRIORITY 2
#define UPDATE_MONITOR_TASK_PRIORITY 2

// Pass readings from the sensor to the onboard monitor
static QueueHandle_t monitor_queue = NULL;

// read sensor
static void periodic_task(void *param)
{
    (void)param;
    const TickType_t frequency = 1000 / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();
    for (int i = -10;; i++)
    {
        vTaskDelayUntil(&lastWakeTime, frequency);

        float sensor_reading = (float)i;
        if (xQueueSend(monitor_queue, (void *)&sensor_reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(MAIN_TAG, "Last reading not received from monitor, overwriting with new value");
            configASSERT(xQueueOverwrite(monitor_queue, (void *)&sensor_reading));
        }
    }
}

static void update_monitor_task(void *param)
{
    (void)param;
    while (1)
    {
        float sensor_reading;
        if (xQueueReceive(monitor_queue, &sensor_reading, portMAX_DELAY))
        {
            printf("MONITOR -- temperature: %f\n", sensor_reading);
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(ble_handler_init());

    monitor_queue = xQueueCreate(1, sizeof(float));

    TaskHandle_t periodic_task_handle = NULL;
    xTaskCreate(&periodic_task, "periodic_task", TASK_STACK_DEPTH, NULL, PERIODIC_TASK_PRIORITY, &periodic_task_handle);
    configASSERT(periodic_task_handle);

    TaskHandle_t update_monitor_task_handle = NULL;
    xTaskCreate(&update_monitor_task, "update_monitor_task", TASK_STACK_DEPTH, NULL, UPDATE_MONITOR_TASK_PRIORITY,
                &update_monitor_task_handle);
    configASSERT(update_monitor_task_handle);

    vTaskDelete(NULL);
}
