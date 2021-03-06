//==================================================================================================
// INCLUDES
//==================================================================================================

#include "debug_heartbeat.h"

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

esp_err_t set_level(uint32_t level);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

static gpio_num_t gpio_pin;
static uint32_t gpio_pin_level;

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

esp_err_t debug_heartbeat_init(gpio_num_t gpio_num)
{
    gpio_pin = gpio_num;

    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    return gpio_config(&io_conf);
}

esp_err_t debug_heartbeat_set(void)
{
    gpio_pin_level = 1;
    return gpio_set_level(gpio_pin, gpio_pin_level);
}

esp_err_t debug_heartbeat_reset(void)
{
    gpio_pin_level = 1;
    return gpio_set_level(gpio_pin, gpio_pin_level);
}

esp_err_t debug_heartbeat_toggle(void)
{
    gpio_pin_level ^= 1;
    return gpio_set_level(gpio_pin, gpio_pin_level);
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================
