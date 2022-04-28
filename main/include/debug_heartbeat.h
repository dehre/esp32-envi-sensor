/*
 * This module helps setting up any GPIO pin as a minimally intrusive debugging tool.
 * The pin can then be connected to an LED, to a logic analyzer, or to an oscilloscope.
 * Good to know: not EVERY pin can be used as heartbeat, see:
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html
 *
 * Example (without error checking):
 * ```c
 * #include "debug_heartbeat.h"
 *
 * int main(void)
 * {
 *     debug_heartbeat_init(GPIO_NUM_25);
 *     while(1)
 *     {
 *         debug_heartbeat_toggle();
 *         vTaskDelay(1000 / portTICK_PERIOD_MS);
 *     }
 * }
 * ```
 */

#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t debug_heartbeat_init(gpio_num_t gpio_num);

esp_err_t debug_heartbeat_set(void);

esp_err_t debug_heartbeat_reset(void);

esp_err_t debug_heartbeat_toggle(void);
