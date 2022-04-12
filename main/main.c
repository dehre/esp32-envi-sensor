#include "ble_handler.h"
#include "esp_err.h"

void app_main(void)
{
    ESP_ERROR_CHECK(ble_handler_init());
}
