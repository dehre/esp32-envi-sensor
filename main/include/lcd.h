#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t lcd_init(void);

void lcd_store_temperature(float temperature);

void lcd_store_humidity(float humidity);

void lcd_select_next_view(void);

void lcd_render(void);
