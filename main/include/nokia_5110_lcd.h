#pragma once

#include <stdint.h>

void nokia_5110_lcd_init(void);

void nokia_5110_lcd_write_temperature(float temperature);

void nokia_5110_lcd_write_humidity(float humidity);
