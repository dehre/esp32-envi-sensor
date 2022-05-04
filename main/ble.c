//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ble.h"

#include "store_float_into_uint8_arr.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

#define ESP_LOG_TAG "ENVI_SENSOR_BLE"
#include "iferr.h"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define SERVICE_INSTANCE_ID 0

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* Attributes Indexes */
enum
{
    IDX_SERVICE,

    IDX_TEMPERATURE_CHARACT,
    IDX_TEMPERATURE_CHARACT_VALUE,

    IDX_HUMIDITY_CHARACT,
    IDX_HUMIDITY_CHARACT_VALUE,

    IDX_COUNT,
};

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static esp_err_t gatts_read_event_handler(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

/*
 * Advertising
 */

// clang-format off
static uint8_t adv_service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the advertised service (0x181A - Environmental Sensing Service)
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x1A, 0x18, 0x00, 0x00,
};
// clang-format on

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = ESP_BLE_CONN_INT_MAX, // maximum value for slave connection interval
    .max_interval = 0xFFFF,               // unspecified value
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid),
    .p_service_uuid = adv_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

/* For the iOS system, please refer to Apple official documents about the BLE advertising parameters
 * restrictions: https://developer.apple.com/library/archive/qa/qa1931/_index.html */
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x0808, // advertising happens every 0x0808 * 0.625ms = 1285ms
    .adv_int_max = 0x0808, // advertising happens every 0x0808 * 0.625ms = 1285ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/*
 * Profile
 */

static struct gatts_profile_inst environmental_sensing_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] =
        {
            .gatts_cb = gatts_profile_event_handler,
            .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        },
};

/*
 * Service and Characteristics
 */

static const uint16_t GATTS_ENVIRONMENTAL_SENSING_SERVICE_UUID = 0x181A;
static const uint16_t GATTS_TEMPERATURE_CHARACT_UUID = 0x2A6E;
static const uint16_t GATTS_HUMIDITY_CHARACT_UUID = 0x2A6F;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t charact_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t charact_property_read = ESP_GATT_CHAR_PROP_BIT_READ;

/* temperature_charact_value and humidity_charact_value hold the last temperature and
 *   humidity reading, respectively */
static uint8_t temperature_charact_value[2];
static uint8_t humidity_charact_value[2];

/* temperature_charact_sentinel_value and humidity_charact_sentinel_value are used to detect
 *   if the ESP_GATTS_READ_EVT has been triggered by a temperature or a humidity reading */
static const uint8_t temperature_charact_sentinel_value[2] = {0x80, 0x00};
static const uint8_t humidity_charact_sentinel_value[2] = {0xFF, 0xFF};

/* Full Database Description - Used to add attributes into the database */
// clang-format off
static const esp_gatts_attr_db_t gatt_db[IDX_COUNT] =
{
    // Service Declaration
    [IDX_SERVICE] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_ENVIRONMENTAL_SENSING_SERVICE_UUID), (uint8_t *)&GATTS_ENVIRONMENTAL_SENSING_SERVICE_UUID}},

    /* Characteristic Declaration */
    [IDX_TEMPERATURE_CHARACT] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&charact_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(charact_property_read), sizeof(charact_property_read), (uint8_t*)&charact_property_read}},

    /* Characteristic Value */
    [IDX_TEMPERATURE_CHARACT_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_TEMPERATURE_CHARACT_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(temperature_charact_sentinel_value), sizeof(temperature_charact_sentinel_value), (uint8_t*)temperature_charact_sentinel_value}},

    /* Characteristic Declaration */
    [IDX_HUMIDITY_CHARACT] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&charact_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(charact_property_read), sizeof(charact_property_read), (uint8_t*)&charact_property_read}},

    /* Characteristic Value */
    [IDX_HUMIDITY_CHARACT_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_HUMIDITY_CHARACT_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(humidity_charact_sentinel_value), sizeof(humidity_charact_sentinel_value), (uint8_t*)humidity_charact_sentinel_value}},
};
// clang-format on

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

esp_err_t ble_init(void)
{
    ESP_LOGI(ESP_LOG_TAG, "%s - initialize", __func__);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        IFERR_RETE(nvs_flash_erase(), "erase flash failed");
        ret = nvs_flash_init();
    }
    IFERR_RETE(ret, "init flash failed");

    IFERR_RETE(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT), "release controller memory failed");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    IFERR_RETE(esp_bt_controller_init(&bt_cfg), "init controller failed");
    IFERR_RETE(esp_bt_controller_enable(ESP_BT_MODE_BLE), "enable controller failed");

    IFERR_RETE(esp_bluedroid_init(), "init bluetooth failed");
    IFERR_RETE(esp_bluedroid_enable(), "enable bluetooth failed");

    IFERR_RETE(esp_ble_gatts_register_callback(gatts_event_handler), "gatts register error");
    IFERR_RETE(esp_ble_gap_register_callback(gap_event_handler), "gap register error");
    IFERR_RETE(esp_ble_gatts_app_register(PROFILE_APP_IDX), "gatts app register error");
    IFERR_RETE(esp_ble_gatt_set_local_mtu(500), "set local MTU failed");
    return ESP_OK;
}

esp_err_t ble_write_temperature(float temperature)
{
    ESP_LOGD(ESP_LOG_TAG, "%s - write %f", __func__, temperature);
    // See: GATT Specification Supplement Datasheet Page 223 Section 3.204
    if (temperature < -273.15 || temperature > 327.67)
    {
        return ESP_ERR_INVALID_ARG;
    }
    store_float_into_uint8_arr(&temperature, temperature_charact_value);
    return ESP_OK;
}

esp_err_t ble_write_humidity(float humidity)
{
    ESP_LOGD(ESP_LOG_TAG, "%s - write %f", __func__, humidity);
    // See: GATT Specification Supplement Datasheet Page 146 Section 3.114
    if (humidity < 0.00 || humidity > 100.00)
    {
        return ESP_ERR_INVALID_ARG;
    }
    store_float_into_uint8_arr(&humidity, humidity_charact_value);
    return ESP_OK;
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static esp_err_t gatts_read_event_handler(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    uint16_t sentinel_value_length;
    const uint8_t *sentinel_value;
    IFERR_RETE(esp_ble_gatts_get_attr_value(param->read.handle, &sentinel_value_length, &sentinel_value),
               "illegal handle %d", param->read.handle);
    assert(sentinel_value_length == 2);

    esp_gatt_rsp_t rsp = {0};
    rsp.attr_value.handle = param->read.handle;
    if (memcmp(temperature_charact_sentinel_value, sentinel_value, sizeof(temperature_charact_sentinel_value)) == 0)
    {
        rsp.attr_value.len = sizeof(temperature_charact_value);
        memcpy(rsp.attr_value.value, temperature_charact_value, sizeof(temperature_charact_value));
    }
    else if (memcmp(humidity_charact_sentinel_value, sentinel_value, sizeof(humidity_charact_sentinel_value)) == 0)
    {
        rsp.attr_value.len = sizeof(humidity_charact_value);
        memcpy(rsp.attr_value.value, humidity_charact_value, sizeof(humidity_charact_value));
    }
    else
    {
        assert(0);
    }

    IFERR_RETE(esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp),
               "failed to send response");
    return ESP_OK;
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_REG_EVT");
        if (param->reg.status != ESP_GATT_OK)
        {
            ESP_LOGE(ESP_LOG_TAG, "reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
        environmental_sensing_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
    }
    for (int idx = 0; idx < PROFILE_NUM; idx++)
    {
        /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == environmental_sensing_profile_tab[idx].gatts_if)
        {
            if (environmental_sensing_profile_tab[idx].gatts_cb)
            {
                environmental_sensing_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT: {
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_REG_EVT");
        IFERR_LOG(esp_ble_gap_set_device_name(BLE_DEVICE_NAME), "set device name failed");
        IFERR_LOG(esp_ble_gap_config_adv_data(&adv_data), "config adv data failed");
        IFERR_LOG(esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_COUNT, SERVICE_INSTANCE_ID),
                  "create attr table failed");
    }
    break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id,
                 param->read.trans_id, param->read.handle);
        IFERR_RETV(gatts_read_event_handler(gatts_if, param), "failed to handle ESP_GATTS_READ_EVT");
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(ESP_LOG_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status,
                 param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
        esp_log_buffer_hex(ESP_LOG_TAG, param->connect.remote_bda, 6);
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = ESP_BLE_CONN_INT_MAX;
        conn_params.min_int = ESP_BLE_CONN_INT_MAX;
        conn_params.timeout = ESP_BLE_CONN_SUP_TOUT_MAX;
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        ESP_LOGI(ESP_LOG_TAG, "ESP_GATTS_CREAT_ATTR_TAB_EVT");
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(ESP_LOG_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            return;
        }
        if (param->add_attr_tab.num_handle != IDX_COUNT)
        {
            ESP_LOGE(ESP_LOG_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_COUNT(%d)",
                     param->add_attr_tab.num_handle, IDX_COUNT);
            return;
        }
        ESP_LOGI(ESP_LOG_TAG, "create attribute table successfully, the number handle = %d\n",
                 param->add_attr_tab.num_handle);
        uint16_t environmental_sensing_handle_table[IDX_COUNT];
        memcpy(environmental_sensing_handle_table, param->add_attr_tab.handles,
               sizeof(environmental_sensing_handle_table));
        esp_ble_gatts_start_service(environmental_sensing_handle_table[IDX_SERVICE]);
        break;
    }
    default:
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGI(ESP_LOG_TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT");
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(ESP_LOG_TAG, "advertising start failed");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(ESP_LOG_TAG,
                 "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT, status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = "
                 "%d, timeout = %d",
                 param->update_conn_params.status, param->update_conn_params.min_int, param->update_conn_params.max_int,
                 param->update_conn_params.conn_int, param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}
