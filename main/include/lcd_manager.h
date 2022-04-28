#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t lcd_manager_init(void);

void lcd_manager_store_temperature(float temperature);

void lcd_manager_store_humidity(float humidity);

void lcd_manager_select_next_view(void);

void lcd_manager_render(void);
