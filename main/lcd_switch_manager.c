//==================================================================================================
// INCLUDES
//==================================================================================================

#include "lcd_switch_manager.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "MAIN"
#include "iferr.h"

// TODO LORIS: header file for all pins?
#define LCD_SWITCH_PIN 21
#define LCD_SWITCH_BIT_MASK (1ULL << LCD_SWITCH_PIN)

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

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
    return ESP_OK;
}

// TODO LORIS: ? unique fn that just disables interrupts, waits 500ms, then re-enables them?
void lcd_switch_manager_start_debounce(void)
{
    gpio_intr_disable(LCD_SWITCH_PIN);
}

void lcd_switch_manager_end_debounce(void)
{
    vTaskDelay(500 / portTICK_PERIOD_MS); // TODO LORIS: #define these 500ms for lcd_switch debounce
    gpio_intr_enable(LCD_SWITCH_PIN);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================
