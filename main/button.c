//==================================================================================================
// INCLUDES
//==================================================================================================

#include "button.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "ENVI_SENSOR_BUTTON"
#include "iferr.h"

// TODO LORIS: header file for all pins?
#define BUTTON_PIN 21
#define BUTTON_BIT_MASK (1ULL << BUTTON_PIN)

#define TASK_STACK_DEPTH 2048
#define TT_PRIORITY_DEBOUNCE_BUTTON 2

#define DEBOUNCE_DELAY_MS 500

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void create_tt_debounce_button(void);

static void tt_debounce_button(void *param);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

// binsemaphore_button_debounce wakes the task responsible for re-enabling interrupts on button
static SemaphoreHandle_t binsemaphore_button_debounce = NULL;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

esp_err_t button_init(gpio_isr_t isr_handler)
{
    gpio_config_t gpio_conf = {0};
    gpio_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_conf.pin_bit_mask = BUTTON_BIT_MASK;
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pull_up_en = 1;
    IFERR_RETE(gpio_config(&gpio_conf), "failed button setup");

    IFERR_RETE(gpio_install_isr_service(0), "failed to install isr_service");
    IFERR_RETE(gpio_isr_handler_add(BUTTON_PIN, isr_handler, NULL), "failed to register isr_handler");

    binsemaphore_button_debounce = xSemaphoreCreateBinary();
    create_tt_debounce_button();

    return ESP_OK;
}

void button_debounce(void)
{
    gpio_intr_disable(BUTTON_PIN);
    xSemaphoreGiveFromISR(binsemaphore_button_debounce, NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void create_tt_debounce_button(void)
{
    TaskHandle_t task_handle = NULL;
    xTaskCreate(tt_debounce_button, "tt_debounce_button", TASK_STACK_DEPTH, NULL, TT_PRIORITY_DEBOUNCE_BUTTON,
                &task_handle);
    configASSERT(task_handle);
}

static void tt_debounce_button(void *param)
{
    while (1)
    {
        while (!xSemaphoreTake(binsemaphore_button_debounce, portMAX_DELAY))
        {
        }
        vTaskDelay(DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS);
        gpio_intr_enable(BUTTON_PIN);
    }
}
