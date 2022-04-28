set(c_SRCS main.c ble_manager.c lcd_manager.c lcd_switch_manager.c ringbuf.c
           store_float_into_uint8_arr.c debug_heartbeat.c)

idf_component_register(SRCS ${c_SRCS} INCLUDE_DIRS include)
