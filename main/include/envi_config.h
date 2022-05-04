/*
 * This header centralizes pins mapping and task priorities across different modules.
 *
 */

#pragma once

#include "hal/gpio_types.h"

//
// Pins Mapping
//
#define BUTTON_PIN GPIO_NUM_21     // Button
#define HEARTBEAT_PIN GPIO_NUM_25  // Logic Analyzer
#define JTAG_TDO GPIO_NUM_15       // JTAG TDO
#define JTAG_TDI GPIO_NUM_12       // JTAG TDI
#define JTAG_TCK GPIO_NUM_13       // JTAG TCK
#define JTAG_TMS GPIO_NUM_14       // JTAG TMS
#define LCD_CLK_PIN GPIO_NUM_18    // LCD Clock (hardcoded in ssd1306 lib)
#define LCD_DIN_PIN GPIO_NUM_23    // LCD MOSI  (hardcoded in ssd1306 lib)
#define LCD_DC_PIN GPIO_NUM_17     // LCD Data Command
#define LCD_CE_PIN GPIO_NUM_5      // LCD Chip Enable
#define LCD_RST_PIN GPIO_NUM_16    // LCD Reset
#define SENSOR_SDA_PIN GPIO_NUM_32 // Sensor SDA
#define SENSOR_SCL_PIN GPIO_NUM_33 // Sensor SCL

//
// Task Priorities
//
#define TASK_PRIORITY_MAIN 1                          // priority of main task, for reference
#define TASK_PRIORITY_MAX (configMAX_PRIORITIES - 1U) // max priority that can be assigned, for reference
#define TASK_PRIORITY_READ_SENSOR 2
#define TASK_PRIORITY_UPDATE_BLE 3
#define TASK_PRIORITY_UPDATE_LCD_RING_BUFFER 3
#define TASK_PRIORITY_DEBOUNCE_BUTTON 3
#define TASK_PRIORITY_RENDER_LCD_VIEW 4

//
// Task Stack Depths (in words)
//
#define TASK_STACK_DEPTH 2048
