//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ble_manager.h"
#include "debug_heartbeat.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lcd_manager.h"
#include "lcd_switch_manager.h"
#include "sht21.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "MAIN"
#include "iferr.h"

//
// Task Priorities
//
#define TT_PRIORITY_MAIN 1                          // priority of main task, for reference
#define TT_PRIORITY_MAX (configMAX_PRIORITIES - 1U) // max priority that can be assigned, for reference
#define TT_PRIORITY_READ_SENSOR 2
#define TT_PRIORITY_UPDATE_BLE 2
#define TT_PRIORITY_UPDATE_LCD_RING_BUFFER 2
#define TT_PRIORITY_RENDER_LCD_VIEW 2

// TODO LORIS: can the value be lower?
#define TASK_STACK_DEPTH 2048

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

static void lcd_switch_isr_handler(void *param);

static void create_task(TaskFunction_t fn, const char *const name, UBaseType_t priority, uint32_t stack_depth);

static void tt_read_sensor(void *param);

static void tt_update_ble(void *param);

static void tt_update_lcd_ring_buffer(void *param);

static void tt_render_lcd_view(void *param);

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
    binqueue_ble = xQueueCreate(1, sizeof(sensor_reading_t));
    binqueue_lcd = xQueueCreate(1, sizeof(sensor_reading_t));
    binsemaphore_lcd_render = xSemaphoreCreateBinary();

    ESP_ERROR_CHECK(sht21_init(0, GPIO_NUM_32, GPIO_NUM_33, sht21_i2c_speed_standard));
    ESP_ERROR_CHECK(ble_manager_init());
    ESP_ERROR_CHECK(lcd_switch_manager_init(lcd_switch_isr_handler));
    ESP_ERROR_CHECK(lcd_manager_init());
    ESP_ERROR_CHECK(debug_heartbeat_init(GPIO_NUM_25));

    create_task(tt_read_sensor, "tt_read_sensor", TT_PRIORITY_READ_SENSOR, TASK_STACK_DEPTH);
    create_task(tt_update_ble, "tt_update_ble", TT_PRIORITY_UPDATE_BLE, TASK_STACK_DEPTH);
    create_task(tt_update_lcd_ring_buffer, "tt_update_lcd_ring_buffer", TT_PRIORITY_UPDATE_LCD_RING_BUFFER,
                TASK_STACK_DEPTH);
    create_task(tt_render_lcd_view, "tt_render_lcd_view", TT_PRIORITY_RENDER_LCD_VIEW,
                CONFIG_STACK_DEPTH_TASK_RENDER_LCD_VIEW);

    vTaskDelete(NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void lcd_switch_isr_handler(void *param)
{
    lcd_switch_manager_debounce();
    lcd_manager_select_next_view();
    xSemaphoreGiveFromISR(binsemaphore_lcd_render, NULL);
}

static void create_task(TaskFunction_t fn, const char *const name, UBaseType_t priority, uint32_t stack_depth)
{
    TaskHandle_t task_handle = NULL;
    xTaskCreate(fn, name, stack_depth, NULL, priority, &task_handle);
    configASSERT(task_handle);
}

static void tt_read_sensor(void *param)
{
    const TickType_t frequency = CONFIG_READ_SENSOR_FREQUENCY_MS / portTICK_PERIOD_MS;
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
        if (xQueueSend(binqueue_ble, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from BLE peripheral, overwriting with new value");
            configASSERT(xQueueOverwrite(binqueue_ble, (void *)&reading));
        }
        if (xQueueSend(binqueue_lcd, (void *)&reading, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGW(ESP_LOG_TAG, "last sensor reading not received from monitor, overwriting with new value");
            configASSERT(xQueueOverwrite(binqueue_lcd, (void *)&reading));
        }
        if (xSemaphoreGive(binsemaphore_lcd_render) != pdTRUE)
        {
            ESP_LOGW(ESP_LOG_TAG, "failed to give binsemaphore_lcd_render");
        }
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

static void tt_update_ble(void *param)
{
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(binqueue_ble, &reading, portMAX_DELAY))
        {
            IFERR_LOG(ble_manager_write_temperature(reading.temperature), "failed to write temperature");
            IFERR_LOG(ble_manager_write_humidity(reading.humidity), "failed to write humidity");
        }
    }
}

static void tt_update_lcd_ring_buffer(void *param)
{
    while (1)
    {
        sensor_reading_t reading;
        if (xQueueReceive(binqueue_lcd, &reading, portMAX_DELAY))
        {
            lcd_manager_store_temperature(reading.temperature);
            lcd_manager_store_humidity(reading.humidity);
        }
    }
}

static void tt_render_lcd_view(void *param)
{
    while (1)
    {
        while (!xSemaphoreTake(binsemaphore_lcd_render, portMAX_DELAY))
        {
        }
        lcd_manager_render();
    }
}
