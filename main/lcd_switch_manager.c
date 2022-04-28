//==================================================================================================
// INCLUDES
//==================================================================================================

#include "lcd_switch_manager.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "MAIN"
#include "iferr.h"

// TODO LORIS: header file for all pins?
#define LCD_SWITCH_PIN 21
#define LCD_SWITCH_BIT_MASK (1ULL << LCD_SWITCH_PIN)

#define TASK_STACK_DEPTH 2048
#define TT_PRIORITY_DEBOUNCE_LCD_SWITCH 2

#define DEBOUNCE_DELAY_MS 500

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void create_tt_debounce_lcd_switch(void);

static void tt_debounce_lcd_switch(void *param);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

// binsemaphore_lcd_switch_debounce wakes the task responsible for re-enabling interrupts on lcd_switch
static SemaphoreHandle_t binsemaphore_lcd_switch_debounce = NULL;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

esp_err_t lcd_switch_manager_init(gpio_isr_t isr_handler)
{
    gpio_config_t gpio_conf = {0};
    gpio_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_conf.pin_bit_mask = LCD_SWITCH_BIT_MASK;
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pull_up_en = 1;
    IFERR_RETE(gpio_config(&gpio_conf), "failed lcd_switch setup");

    IFERR_RETE(gpio_install_isr_service(0), "failed to install isr_service");
    IFERR_RETE(gpio_isr_handler_add(LCD_SWITCH_PIN, isr_handler, NULL), "failed to register isr_handler");

    binsemaphore_lcd_switch_debounce = xSemaphoreCreateBinary();
    create_tt_debounce_lcd_switch();

    return ESP_OK;
}

void lcd_switch_manager_debounce(void)
{
    gpio_intr_disable(LCD_SWITCH_PIN);
    xSemaphoreGiveFromISR(binsemaphore_lcd_switch_debounce, NULL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void create_tt_debounce_lcd_switch(void)
{
    TaskHandle_t task_handle = NULL;
    xTaskCreate(tt_debounce_lcd_switch, "tt_debounce_lcd_switch", TASK_STACK_DEPTH, NULL,
                TT_PRIORITY_DEBOUNCE_LCD_SWITCH, &task_handle);
    configASSERT(task_handle);
}

static void tt_debounce_lcd_switch(void *param)
{
    while (1)
    {
        while (!xSemaphoreTake(binsemaphore_lcd_switch_debounce, portMAX_DELAY))
        {
        }
        vTaskDelay(DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS);
        gpio_intr_enable(LCD_SWITCH_PIN);
    }
}
