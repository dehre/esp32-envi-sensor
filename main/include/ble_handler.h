#pragma once

#include "esp_err.h"

esp_err_t ble_handler_init(void);

esp_err_t ble_handler_write_temperature(float temperature);
