#pragma once

#include "esp_err.h"

#define BLE_DEVICE_NAME "Envi Sensor" // device name shown when advertising

esp_err_t ble_manager_init(void);

esp_err_t ble_manager_write_temperature(float temperature);
