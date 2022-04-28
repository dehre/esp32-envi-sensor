//==================================================================================================
// INCLUDES
//==================================================================================================

#include "lcd_manager.h"
#include "ringbuf.h"
#include "ssd1306.h"

#include "esp_log.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "LCD_MANAGER"

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

// TODO LORIS: put this #define in main.h and update to 120
#define ringbuf_lcd_data_len_ 40

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void initialize_my_font_6x8(void);

static void clear_line(uint8_t x_pos, uint8_t y_pos);

static void render_last_readings(void);

static void render_historical_temperature(void);

static void render_historical_humidity(void);

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

/* Ring-buffers for temperature and humidity */
static ringbuf_t ringbuf_lcd_temperature;
static ringbuf_t ringbuf_lcd_humidity;

/* Memory reserved for holding ring-buffers' data */
static float ringbuf_lcd_temperature_data_[ringbuf_lcd_data_len_];
static float ringbuf_lcd_humidity_data_[ringbuf_lcd_data_len_];

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

// TODO LORIS: return esp_err_t for consistency
void lcd_manager_init(void)
{
    ringbuf_lcd_temperature = ringbuf_init(ringbuf_lcd_temperature_data_, ringbuf_lcd_data_len_);
    ringbuf_lcd_humidity = ringbuf_init(ringbuf_lcd_humidity_data_, ringbuf_lcd_data_len_);
    initialize_my_font_6x8();
    ssd1306_setFixedFont(my_font_6x8);
    pcd8544_84x48_spi_init(RST_PIN, CE_PIN, DC_PIN);
    ssd1306_clearScreen();
}

void lcd_manager_store_temperature(float temperature)
{
    ringbuf_put(&ringbuf_lcd_temperature, temperature);
}

void lcd_manager_store_humidity(float humidity)
{
    ringbuf_put(&ringbuf_lcd_humidity, humidity);
}

// TODO LORIS: perhaps an enum for lcd_view?
void lcd_manager_render(uint8_t lcd_view)
{
    if (lcd_view > 2)
        return;

    switch (lcd_view)
    {
    case 0:
        return render_last_readings();
    case 1:
        return render_historical_temperature();
    case 2:
        return render_historical_humidity();
    default:
        assert(0);
    }
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

// TODO LORIS: need it?
static void clear_line(uint8_t x_pos, uint8_t y_pos)
{
    char line_buffer[SCREEN_WIDTH + 1] = {0};
    for (uint32_t i = 0; i < SCREEN_WIDTH; i++)
    {
        line_buffer[i] = ' ';
    }
    ssd1306_printFixed(x_pos, y_pos, line_buffer, STYLE_NORMAL);
}

static void render_last_readings(void)
{
    ssd1306_clearScreen();
    ssd1306_printFixed(24, 0, "Envi", STYLE_ITALIC);
    ssd1306_printFixed(16, 8, "Sensor", STYLE_ITALIC);

    float temperature;
    float humidity;
    uint8_t success = 0x01;
    success &= ringbuf_get(&ringbuf_lcd_temperature, &temperature);
    success &= ringbuf_get(&ringbuf_lcd_humidity, &humidity);
    if (success == 0)
    {
        ssd1306_printFixed(0, 24, "No data yet", STYLE_NORMAL);
        return;
    }

    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f'C", "Temp:", temperature);
    ssd1306_printFixed(0, 24, line_buffer, STYLE_NORMAL);
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f %%", "Hum:", humidity);
    ssd1306_printFixed(0, 40, line_buffer, STYLE_NORMAL);
}

static void render_historical_temperature(void)
{
    ssd1306_clearScreen();
    ssd1306_printFixed(8, 0, "Temperature", STYLE_ITALIC);
    ssd1306_printFixed(16, 8, "Analysis", STYLE_ITALIC);

    float sorted_temps[ringbuf_lcd_data_len_];
    size_t sorted_temps_len = ringbuf_getallsorted(&ringbuf_lcd_temperature, sorted_temps);
    if (sorted_temps_len == 0)
    {
        ssd1306_printFixed(0, 24, "No data yet", STYLE_NORMAL);
        return;
    }

    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f'C", "Min:", sorted_temps[0]);
    ssd1306_printFixed(0, 24, line_buffer, STYLE_NORMAL);
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f'C", "Med:", sorted_temps[((sorted_temps_len - 1) / 2)]);
    ssd1306_printFixed(0, 32, line_buffer, STYLE_NORMAL);
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f'C", "Max:", sorted_temps[(sorted_temps_len - 1)]);
    ssd1306_printFixed(0, 40, line_buffer, STYLE_NORMAL);
}

static void render_historical_humidity(void)
{
    ssd1306_clearScreen();
    ssd1306_printFixed(16, 0, "Humidity", STYLE_ITALIC);
    ssd1306_printFixed(16, 8, "Analysis", STYLE_ITALIC);

    float sorted_humids[ringbuf_lcd_data_len_];
    size_t sorted_humids_len = ringbuf_getallsorted(&ringbuf_lcd_humidity, sorted_humids);
    if (sorted_humids_len == 0)
    {
        ssd1306_printFixed(0, 24, "No data yet", STYLE_NORMAL);
        return;
    }

    char line_buffer[SCREEN_WIDTH + 1];
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f %%", "Min:", sorted_humids[0]);
    ssd1306_printFixed(0, 24, line_buffer, STYLE_NORMAL);
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f %%", "Med:", sorted_humids[((sorted_humids_len - 1) / 2)]);
    ssd1306_printFixed(0, 32, line_buffer, STYLE_NORMAL);
    snprintf(line_buffer, SCREEN_WIDTH + 1, "%-5s %.1f %%", "Max:", sorted_humids[(sorted_humids_len - 1)]);
    ssd1306_printFixed(0, 40, line_buffer, STYLE_NORMAL);
}
