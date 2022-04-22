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

#define MY_FONT_6x8_LEN 581 // enough memory to hold a copy of ssd1306xled_font6x8
#define APOSTROPHE_IDX 46   // index of `'` in ssd1306xled_font6x8

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

static void initialize_my_font_6x8(void);

static void clear_line(uint8_t x_pos, uint8_t y_pos);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

/*
 * The symbol `°` is in the extended part of ASCII table and isn't included in ssd1306xled_font6x8.
 * Without modifying the original library, we copy ssd1306xled_font6x8 into my_font_6x8 and
 *   replace the bitmap for `'` with the bitmap for `°`.
 * See: https://github.com/lexus2k/ssd1306/issues/139
 */
static uint8_t my_font_6x8[MY_FONT_6x8_LEN];

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

void nokia_5110_lcd_init(void)
{
    initialize_my_font_6x8();
    ssd1306_setFixedFont(my_font_6x8);
    pcd8544_84x48_spi_init(RST_PIN, CE_PIN, DC_PIN);
    ssd1306_clearScreen();
    ssd1306_printFixed(8, 8, "Envi Sensor", STYLE_ITALIC);
}

void nokia_5110_lcd_write_temperature(float temperature)
{
    clear_line(0, 24);
    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f'C", "Temp:", temperature);
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

static void initialize_my_font_6x8(void)
{
    uint8_t my_bitmap[] = {0x00, 0x00, 0x06, 0x06, 0x00, 0x00}; // bitmap for °
    memcpy(my_font_6x8, ssd1306xled_font6x8, MY_FONT_6x8_LEN);
    for (size_t i = APOSTROPHE_IDX, j = 0; j < sizeof(my_bitmap); i++, j++)
    {
        my_font_6x8[i] = my_bitmap[j];
    }
}

static void clear_line(uint8_t x_pos, uint8_t y_pos)
{
    char line_buffer[SCREEN_WIDTH + 1] = {0};
    for (uint32_t i = 0; i < SCREEN_WIDTH; i++)
    {
        line_buffer[i] = ' ';
    }
    ssd1306_printFixed(x_pos, y_pos, line_buffer, STYLE_NORMAL);
}
