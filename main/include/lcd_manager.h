#pragma once

#include <stdint.h>

void lcd_manager_init(void);

void lcd_manager_write_temperature(float temperature);

void lcd_manager_write_humidity(float humidity);
