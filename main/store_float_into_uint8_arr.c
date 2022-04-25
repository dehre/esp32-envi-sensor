#include "store_float_into_uint8_arr.h"
#include <stdint.h>

void store_float_into_uint8_arr(const float *f32_value, uint8_t arr[2])
{
    float f32_val = *f32_value;
    int16_t i16_val = (int16_t)(f32_val * 100);
    uint8_t msb = (uint8_t)(i16_val >> 8);
    uint8_t lsb = (uint8_t)(i16_val & 0xFF);
    arr[1] = msb;
    arr[0] = lsb;
}
