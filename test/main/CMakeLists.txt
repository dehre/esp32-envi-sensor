set(main_DIR ../../main)
set(main_c_SRCS ${main_DIR}/utils.c ${main_DIR}/ringbuf.c)
set(main_include_DIRS ${main_DIR}/include)

set(test_c_SRCS unit_tests.c)

idf_component_register(
    SRCS ${test_c_SRCS} ${main_c_SRCS}
    INCLUDE_DIRS ${main_include_DIRS}
    REQUIRES unity)
