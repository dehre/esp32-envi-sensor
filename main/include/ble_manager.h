#pragma once

#include "esp_err.h"

esp_err_t ble_manager_init(void);

esp_err_t ble_manager_write_temperature(float temperature);
