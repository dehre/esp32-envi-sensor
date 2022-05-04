set(c_SRCS
    ble.c
    button.c
    debug_heartbeat.c
    lcd.c
    main.c
    ringbuf.c
    store_float_into_uint8_arr.c)

idf_component_register(SRCS ${c_SRCS} INCLUDE_DIRS include)
