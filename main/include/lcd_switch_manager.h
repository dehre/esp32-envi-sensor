#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t lcd_switch_manager_init(gpio_isr_t isr_handler);

void lcd_switch_manager_start_debounce(void);

void lcd_switch_manager_end_debounce(void);
