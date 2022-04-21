# ESP32 Envi Sensor

Use an ESP32 board to grab temperature and humidity readings from a sensor and output them
via BLE as a GATT server.  
Additionally, the readings are shown on a Nokia 5110 Display.

## Bill of Materials

- ESP32 Dev Board

- SHT21 Humidity and Temperature Sensor (a GY-21 module in my case)

- Nokia 5110 Display

- ESP-Prog JTAG Adapter (optional, useful for development)

- Breadboard and cables as usual

TODO LORIS: upload pic

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

## BLE Setup

The sensor telemetry will be advertised over BLE as a GATT Environmental Sensing Service (GATT Assigned Number 0x181A) with multiple GATT Characteristics.
Each Characteristic represents a sensor reading and contains the most current sensor value(s), for example, Temperature (0x2A6E) or Humidity (0x2A6F).

Each GATT Characteristic defines how the data should be represented.
To represent the data accurately, the sensor readings need to be modified.
For example, using the SHT21 library, the temperature is captured as floating point number.
However, the Temperature GATT Characteristic (0x2A6E) requires a signed 16-bit value (-32,768 32,767).
To maintain precision, the captured value (e.g., 22.21 °C) is multiplied by 100 to convert it to an integer (e.g., 2221). The ESP32 will then handle converting the value back to the original value with the correct precision.

## Writing temperature and humidity to the BLE peripheral

**GATT Specification Supplement Datasheet Page 23 Section 2.1: Scalar Values**
When a characteristic field represents a scalar value and unless otherwise specified by the characteristic
definition, the represented value is related to the raw value by the following equations, where the M
coefficient, d, and b exponents are defined per field of characteristic:
R = C \* M \* 10^d \* 2^b

**GATT Specification Supplement Datasheet Page 223 Section 3.204: Temperature**
Represented values: M = 1, d = -2, b = 0
Unit is degrees Celsius with a resolution of 0.01 degrees Celsius.
Allowed range is: -273.15 to 327.67.
A value of 0x8000 represents ‘value is not known’.
All other values are prohibited.

**GATT Specification Supplement Datasheet Page 146 Section 3.114: Humidity**
Represented values: M = 1, d = -2, b = 0
Unit is in percent with a resolution of 0.01 percent.
Allowed range is: 0.00 to 100.00
A value of 0xFFFF represents ‘value is not known’.
All other values are prohibited.

## BLE Events Lifecycle

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
# Reading characteristic 3 times
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
