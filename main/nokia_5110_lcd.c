//==================================================================================================
// INCLUDES
//==================================================================================================

#include "nokia_5110_lcd.h"
#include "ssd1306.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define SCREEN_WIDTH (84 / CHAR_WIDTH)
#define SCREEN_HEIGHT (48 / CHAR_HEIGHT)

// #define GND_PIN   -> GND
// #define LIGHT_PIN -> 470Ω -> GND (leave it unconnected if you don't want to use it)
// #define VCC_PIN   -> 3V3

#define CLK_PIN 18 // Clock (hardcoded in ssd1306 lib)
#define DIN_PIN 23 // MOSI  (hardcoded in ssd1306 lib)
#define DC_PIN 17  // Data Command
#define CE_PIN 5   // Chip Enable
#define RST_PIN 16 // Reset

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void clear_line(uint8_t x_pos, uint8_t y_pos);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

void nokia_5110_lcd_init(void)
{
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    pcd8544_84x48_spi_init(RST_PIN, CE_PIN, DC_PIN);
    ssd1306_clearScreen();
    ssd1306_printFixed(8, 8, "Envi Sensor", STYLE_ITALIC);
}

void nokia_5110_lcd_write_temperature(float temperature)
{
    // TODO LORIS: find a workaround for the `°` symbol?
    clear_line(0, 24);
    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f°C", "Temp:", temperature);
    ssd1306_printFixed(0, 24, line_buffer, STYLE_NORMAL);
}

void nokia_5110_lcd_write_humidity(float humidity)
{
    clear_line(0, 35);
    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f %%", "Hum:", humidity);
    ssd1306_printFixed(0, 35, line_buffer, STYLE_NORMAL);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static void clear_line(uint8_t x_pos, uint8_t y_pos)
{
    char line_buffer[SCREEN_WIDTH + 1] = {0};
    for (uint32_t i = 0; i < SCREEN_WIDTH; i++)
    {
        line_buffer[i] = ' ';
    }
    ssd1306_printFixed(x_pos, y_pos, line_buffer, STYLE_NORMAL);
}
