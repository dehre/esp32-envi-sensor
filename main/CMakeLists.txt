set(c_SRCS main.c ble_manager.c lcd_manager.c utils.c)

idf_component_register(SRCS ${c_SRCS} INCLUDE_DIRS include)
