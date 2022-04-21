# ESP32 Envi Sensor

Use an ESP32 board to grab temperature and humidity readings from a sensor and output them
via BLE as a GATT server.  
Additionally, the readings are shown on a Nokia 5110 Display.

## Bill of Materials

- ESP32-DevKitC V4

  - features two 32-bit CPU cores, 4MB Flash, 520KB SRAM, Wi-Fi, Bluetooth LE,
    and a whole bunch of peripherals

- SHT21 Humidity and Temperature Sensor (a GY-21 module in my case)

- Nokia 5110 Display

- ESP-Prog JTAG Adapter (optional, useful for development)

- Breadboard and cables as usual

TODO LORIS: upload picture

## High-Level Overview

TODO LORIS

## Hardware Connection

The connection between ESP Board and the other components is as follows:

```
   ________________                        ________________
  |                |                      |                |
  |              32|______________________|SDA             |
  |              33|______________________|SCL             |
  |             3V3|______________________|VIN             |
  |             GND|______________________|GND             |
  |                |                      |                |
  |                |                      |__SHT21_Sensor__|
  |                |
  |                |                       ________________
  |                |                      |                |
  |              16|______________________|RST             |
  |               5|______________________|CE              |
  |              17|______________________|DC              |
  |              23|______________________|DIN             |
  |              18|______________________|CLK             |
  |             3V3|______________________|VCC             |
  |                |                      |LIGHT           |
  |             GND|______________________|GND             |
  |                |                      |                |
  |                |                      |_Nokia_5110_LCD_|
  |                |
  |                |                       ________________
  |                |                      |                |
  |              15|______________________|TDO             |
  |              12|______________________|TDI             |
  |              13|______________________|TCK             |
  |              14|______________________|TMS             |
  |             GND|______________________|GND             |
  |                |                      |                |
  |_____ESP32______|                      |__JTAG_Adapter__|
```

The JTAG Adapter is of course be removed after development.

## External Libraries

- [sht21](https://github.com/dehre/sht21) - driver for the temperature and humidity sensor, which I wrote myself

- [ssd1306](https://github.com/lexus2k/ssd1306) - driver for the Nokia 5110 display

## BLE Setup

The SHT21 sensor readings are advertised over BLE as a GATT Environmental Sensing Service (GATT Assigned Number 0x181A) with two GATT Characteristics.
The service has two Characteristics, one representing temperature (0x2A6E) and one representing Humidity (0x2A6F).

Each GATT Characteristic defines how the data should be represented:

- _Temperature_

The data type is a 16-bit signed integer.
Unit is degrees Celsius with a resolution of 0.01 degrees Celsius.
Allowed range is: -273.15 to 327.67.
A value of 0x8000 represents 'value is not known'.
All other values are prohibited.
See `GATT Specification Supplement Datasheet Page 223 Section 3.204`.

- _Humidity_

The data type is a 16-bit unsigned integer.
Unit is in percent with a resolution of 0.01 percent.
Allowed range is: 0.00 to 100.00
A value of 0xFFFF represents 'value is not known'.
All other values are prohibited.
See `GATT Specification Supplement Datasheet Page 223 Section 3.204`.

Using the `sht21` library, both temperature and humidity are retrieved as floating point numbers.
The Temperature GATT Characteristic, however, requires a signed 16-bit value (-32768 32767), so the captured value (e.g. 9.87°C) is multiplied by 100, then converted to an integer (e.g. 987).  
Similar reasoning goes for the Humidity GATT Characteristic.

## BLE Events Lifecycle

This log shows the order in which BLE events are triggered when a BLE-client (i.e. smartphone) connects to the ESP32. It might be useful for debugging.

```sh
#
# Booting application
#

W (937) gatts_event_handler: ESP_GATTS_REG_EVT
W (947) gatts_profile_event_handler: ESP_GATTS_REG_EVT
W (947) gatts_profile_event_handler: ESP_GATTS_CREAT_ATTR_TAB_EVT:
W (957) gap_event_handler: ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT
W (967) gatts_profile_event_handler: ESP_GATTS_START_EVT
W (977) gap_event_handler: ESP_GAP_BLE_ADV_START_COMPLETE_EVT

#
# Connecting to application from BLE client
#

W (17257) gatts_profile_event_handler: ESP_GATTS_CONNECT_EVT
W (17347) gatts_profile_event_handler: ESP_GATTS_MTU_EVT
W (17677) gap_event_handler: ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT

#
# Reading temperature characteristic 3 times
#

W (20197) gatts_profile_event_handler: ESP_GATTS_READ_EVT
W (22717) gatts_profile_event_handler: ESP_GATTS_READ_EVT
W (23587) gatts_profile_event_handler: ESP_GATTS_READ_EVT

#
# Disconnecting from application
#

W (27577) gatts_profile_event_handler: ESP_GATTS_DISCONNECT_EVT
W (27597) gap_event_handler: ESP_GAP_BLE_ADV_START_COMPLETE_EVT
```
