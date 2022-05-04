#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t button_init(gpio_isr_t isr_handler);

void button_debounce(void);
