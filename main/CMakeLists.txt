set(c_SRCS main.c ble_handler.c)

idf_component_register(SRCS ${c_SRCS} INCLUDE_DIRS include)
