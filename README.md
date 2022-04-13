# ESP32 Envi Sensor

## Architecture

## Connection

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

# Old Stuff

| Supported Targets | ESP32 | ESP32-C3 |
| ----------------- | ----- | -------- |

ESP-IDF Gatt Server Service Table Demo
===============================================

This demo shows how to create a GATT service with an attribute table defined in one place. Provided API releases the user from adding attributes one by one as implemented in BLUEDROID. A demo of the other method to create the attribute table is presented in [gatt_server_demo](../gatt_server).

Please check the [tutorial](tutorial/Gatt_Server_Service_Table_Example_Walkthrough.md) for more information about this example.
