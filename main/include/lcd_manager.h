#pragma once

#include <stdint.h>

void lcd_manager_init(void);

void lcd_manager_store_temperature(float temperature);

void lcd_manager_store_humidity(float humidity);

void lcd_manager_render(uint8_t lcd_view);
