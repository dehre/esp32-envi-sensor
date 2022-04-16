#pragma once

#include <stdint.h>

void store_float_into_uint8_arr(uint8_t (*arr)[2], const float *f32_value);

void read_float_from_uint8_arr(const uint8_t (*arr)[2], float *f32_value);
