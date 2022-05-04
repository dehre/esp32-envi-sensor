//==================================================================================================
// INCLUDES
//==================================================================================================

#include "lcd.h"

#include "envi_config.h"
#include "ringbuf.h"

#include "esp_log.h"
#include "ssd1306.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "ENVI_SENSOR_LCD"

#define MY_FONT_6x8_LEN 581 // enough memory to hold a copy of ssd1306xled_font6x8
#define APOSTROPHE_IDX 46   // index of `'` in ssd1306xled_font6x8

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define SCREEN_WIDTH (84 / CHAR_WIDTH)
#define SCREEN_HEIGHT (48 / CHAR_HEIGHT)

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

typedef enum
{
    LCD_VIEW_CURRENT_READINGS = 0,
    LCD_VIEW_TEMPERATURE_ANALYSIS,
    LCD_VIEW_HUMIDITY_ANALYSIS,
    LCD_VIEW_COUNT
} lcd_view_t;

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static void initialize_my_font_6x8(void);

static void render_current_readings(void);

static void render_temperature_analysis(void);

static void render_humidity_analysis(void);

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
static float ringbuf_lcd_temperature_data_[CONFIG_LCD_RINGBUF_DATA_LEN];
static float ringbuf_lcd_humidity_data_[CONFIG_LCD_RINGBUF_DATA_LEN];

// lcd_view determines which view is rendered on the lcd
static lcd_view_t lcd_view = LCD_VIEW_CURRENT_READINGS;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

esp_err_t lcd_init(void)
{
    ringbuf_lcd_temperature = ringbuf_init(ringbuf_lcd_temperature_data_, CONFIG_LCD_RINGBUF_DATA_LEN);
    ringbuf_lcd_humidity = ringbuf_init(ringbuf_lcd_humidity_data_, CONFIG_LCD_RINGBUF_DATA_LEN);
    initialize_my_font_6x8();
    ssd1306_setFixedFont(my_font_6x8);
    pcd8544_84x48_spi_init(LCD_RST_PIN, LCD_CE_PIN, LCD_DC_PIN);
    ssd1306_clearScreen();
    return ESP_OK;
}

void lcd_store_temperature(float temperature)
{
    ringbuf_put(&ringbuf_lcd_temperature, temperature);
}

void lcd_store_humidity(float humidity)
{
    ringbuf_put(&ringbuf_lcd_humidity, humidity);
}

void lcd_select_next_view(void)
{
    lcd_view = (lcd_view + 1) % LCD_VIEW_COUNT;
}

void lcd_render(void)
{
    switch (lcd_view)
    {
    case LCD_VIEW_CURRENT_READINGS:
        return render_current_readings();
    case LCD_VIEW_TEMPERATURE_ANALYSIS:
        return render_temperature_analysis();
    case LCD_VIEW_HUMIDITY_ANALYSIS:
        return render_humidity_analysis();
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

static void render_current_readings(void)
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

static void render_temperature_analysis(void)
{
    ssd1306_clearScreen();
    ssd1306_printFixed(8, 0, "Temperature", STYLE_ITALIC);
    ssd1306_printFixed(16, 8, "Analysis", STYLE_ITALIC);

    static float sorted_temps[CONFIG_LCD_RINGBUF_DATA_LEN];
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

static void render_humidity_analysis(void)
{
    ssd1306_clearScreen();
    ssd1306_printFixed(16, 0, "Humidity", STYLE_ITALIC);
    ssd1306_printFixed(16, 8, "Analysis", STYLE_ITALIC);

    static float sorted_humids[CONFIG_LCD_RINGBUF_DATA_LEN];
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
