/*
 * A ring-buffer for storing floats.
 * No allocatios are made on the heap; instead, memory is provided by the application writer.
 * It's safe to use with multiple producers and multiple consumers.
 *
 * Example (without error checking):
 * ```c
 * #include "ringbuf.h"
 *
 * static float ringbuf_data_[20];
 *
 * int main(void)
 * {
 *     ringbuf rbuf = ringbuf_init(ringbuf_data_, 20);
 *
 *     ringbuf_put(&rbuf, 5);
 *
 *     float value;
 *     ringbuf_get(&rbuf, &value);
 *
 *     float all_values[20];
 *     ringbuf_getallsorted(&rbuf, all_values);
 * }
 * ```
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stddef.h>

typedef struct
{
    float *data;
    size_t capacity;
    size_t get_idx;
    SemaphoreHandle_t mutex;
} ringbuf;

/*
 * ringbuf_init creates a new ring-buffer.
 * It assumes dst is provided by the application writer and exists for the entire lifetime of the program.
 * It returns the new ringbuf.
 */
ringbuf ringbuf_init(float dst[], size_t dst_len);

/*
 * ringbuf_put adds a new item to the ring-buffer, overwriting the oldest one if necessary.
 */
void ringbuf_put(ringbuf *rbuf, float new_item);

/*
 * ringbuf_get gets the last item added to the ring-buffer.
 * It returns the number of items retrieved, i.e. 0 if the ring-buffer is empty, 1 otherwise.
 */
size_t ringbuf_get(ringbuf *rbuf, float *dst);

/*
 * ringbuf_getallsorted gets all the items stored in the ring-buffer.
 * It assumes dst is capable of holding all these items.
 * It returns the number of items retrieved.
 */
size_t ringbuf_getallsorted(ringbuf *rbuf, float dst[]);
