set(c_SRCS main.c ble_manager.c nokia_5110_lcd.c utils.c)

idf_component_register(SRCS ${c_SRCS} INCLUDE_DIRS include)
