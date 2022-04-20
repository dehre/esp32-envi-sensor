#include "unity.h"
#include "utils.h"
#include <stdint.h>

TEST_CASE("should_store_negative_values", "[store_float_into_uint8_arr]")
{
    float f32_value = -18.3;
    uint8_t uint8_arr[2];

    int16_t expected = ((float)-18.3 * (float)100.0);
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should_store_zero", "[store_float_into_uint8_arr]")
{
    float f32_value = 0;
    uint8_t uint8_arr[2];

    int16_t expected = ((float)0.0 * (float)100.0);
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should_store_positive_values", "[store_float_into_uint8_arr]")
{
    float f32_value = 23.78;
    uint8_t uint8_arr[2];

    int16_t expected = ((float)23.78 * (float)100.0);
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should_keep_2_digits_after_the_decimal_point", "[store_float_into_uint8_arr]")
{
    float f32_value = 9.87654321;
    uint8_t uint8_arr[2];

    int16_t expected = ((float)9.87 * (float)100.0);
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[store_float_into_uint8_arr]", false);
    UNITY_END();
}
