#pragma once

#include "esp_err.h"

#define BLE_DEVICE_NAME "Envi Sensor" // device name shown when advertising

esp_err_t ble_init(void);

esp_err_t ble_write_temperature(float temperature);

esp_err_t ble_write_humidity(float humidity);
