#include "ringbuf.h"
#include "store_float_into_uint8_arr.h"
#include "unity.h"
#include <stdint.h>

//==================================================================================================
// store_float_into_uint8_arr
//==================================================================================================

TEST_CASE("should store negative values", "[store_float_into_uint8_arr]")
{
    // Arrange
    float f32_value = -18.3;
    uint8_t uint8_arr[2];

    // Act
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    // Assert
    int16_t expected = ((float)-18.3 * (float)100.0);
    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should store zero", "[store_float_into_uint8_arr]")
{
    // Arrange
    float f32_value = 0;
    uint8_t uint8_arr[2];

    // Act
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    // Assert
    int16_t expected = ((float)0.0 * (float)100.0);
    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should store positive values", "[store_float_into_uint8_arr]")
{
    // Arrange
    float f32_value = 23.78;
    uint8_t uint8_arr[2];

    // Act
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    // Assert
    int16_t expected = ((float)23.78 * (float)100.0);
    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

TEST_CASE("should keep only 2 digits after the decimal point", "[store_float_into_uint8_arr]")
{
    // Arrange
    float f32_value = 9.87654321;
    uint8_t uint8_arr[2];

    // Act
    store_float_into_uint8_arr(&f32_value, uint8_arr);

    // Assert
    int16_t expected = ((float)9.87 * (float)100.0);
    TEST_ASSERT_EQUAL_HEX8(expected & 0xff, uint8_arr[0]);
    TEST_ASSERT_EQUAL_HEX8(expected >> 8, uint8_arr[1]);
}

//==================================================================================================
// ringbuf
//==================================================================================================

TEST_CASE("should get no item, if no item hasn't been added into the ring-buffer yet", "[ringbuf]")
{
    // Arrange
    float ringbuf_data_[3];
    ringbuf_t rbuf = ringbuf_init(ringbuf_data_, 3);

    // Act
    float actual;
    size_t get_count = ringbuf_get(&rbuf, &actual);

    // Assert
    TEST_ASSERT_EQUAL_UINT(0, get_count);
}

TEST_CASE("should get the last item added to the ring-buffer", "[ringbuf]")
{
    // Arrange
    float ringbuf_data_[3];
    ringbuf_t rbuf = ringbuf_init(ringbuf_data_, 3);
    ringbuf_put(&rbuf, 5.43);
    ringbuf_put(&rbuf, 23.29);

    // Act
    float actual;
    size_t get_count = ringbuf_get(&rbuf, &actual);

    // Assert
    TEST_ASSERT_EQUAL_UINT(1, get_count);
    TEST_ASSERT_EQUAL_FLOAT(23.29, actual);
}

TEST_CASE("should get all the items sorted in ascending order", "[ringbuf]")
{
    // Arrange
    float ringbuf_data_[5];
    ringbuf_t rbuf = ringbuf_init(ringbuf_data_, 5);
    ringbuf_put(&rbuf, 0.8);
    ringbuf_put(&rbuf, -18.63);
    ringbuf_put(&rbuf, 33.1);

    // Act
    float actuals[5];
    size_t get_count = ringbuf_getallsorted(&rbuf, actuals);

    // Assert
    TEST_ASSERT_EQUAL_UINT(3, get_count);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(((float[]){-18.63, 0.8, 33.1}), actuals, 3);
}

TEST_CASE("should overwrite the oldest item, if the ring-buffer is full", "[ringbuf]")
{
    // Arrange
    float ringbuf_data_[3];
    ringbuf_t rbuf = ringbuf_init(ringbuf_data_, 3);
    ringbuf_put(&rbuf, 5.43);
    ringbuf_put(&rbuf, 23.29);
    ringbuf_put(&rbuf, -7.2);
    ringbuf_put(&rbuf, 0.4);

    // Act
    float actual;
    size_t get_count;
    get_count = ringbuf_get(&rbuf, &actual);

    // Assert
    TEST_ASSERT_EQUAL_UINT(1, get_count);
    TEST_ASSERT_EQUAL_FLOAT(0.4, actual);

    // Act
    float actuals[3];
    get_count = ringbuf_getallsorted(&rbuf, actuals);

    // Assert
    TEST_ASSERT_EQUAL_UINT(3, get_count);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(((float[]){-7.2, 0.4, 23.29}), actuals, 3);
}

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[store_float_into_uint8_arr]", false);
    unity_run_tests_by_tag("[ringbuf]", false);
    UNITY_END();
}
