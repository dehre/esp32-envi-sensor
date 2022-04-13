#include "ble_handler.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TASK_STACK_DEPTH 2048

#define MAIN_TASK_PRIORITY 1                          // for reference
#define MAX_TASK_PRIORITY (configMAX_PRIORITIES - 1U) // for reference
#define PERIODIC_TASK_PRIORITY 2

static void periodic_task(void *param)
{
    (void)param;
    const TickType_t frequency = 1000 / portTICK_PERIOD_MS;
    TickType_t lastWakeTime = xTaskGetTickCount();
    for (int i = -10;; i++)
    {
        vTaskDelayUntil(&lastWakeTime, frequency);
        ble_handler_write_temperature(i);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(ble_handler_init());

    TaskHandle_t periodic_task_handle = NULL;
    xTaskCreate(&periodic_task, "periodic_task", TASK_STACK_DEPTH, NULL, PERIODIC_TASK_PRIORITY, &periodic_task_handle);
    configASSERT(periodic_task_handle);

    vTaskDelete(NULL);
}
