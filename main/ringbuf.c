//==================================================================================================
// INCLUDES
//==================================================================================================

#include "ringbuf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

//==================================================================================================
// DEFINES - MACROS
//==================================================================================================

/* These defines help with operators' precedence without cluttering the code too
 * much */
#define rbuf_data (rbuf->data)
#define rbuf_capacity (rbuf->capacity)
#define rbuf_get_idx (rbuf->get_idx)
#define rbuf_mutex (rbuf->mutex)

//==================================================================================================
// ENUMS - STRUCTS - TYPEDEFS
//==================================================================================================

//==================================================================================================
// STATIC PROTOTYPES
//==================================================================================================

static int compare_floats(const void *a, const void *b);

//==================================================================================================
// STATIC VARIABLES
//==================================================================================================

//==================================================================================================
// GLOBAL FUNCTIONS
//==================================================================================================

ringbuf_t ringbuf_init(float dst[], size_t dst_len)
{
    for (size_t i = 0; i < dst_len; i++)
    {
        dst[i] = NAN;
    }
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    assert(mutex);
    ringbuf_t rbuf = {.data = dst, .capacity = dst_len, .get_idx = dst_len - 1, .mutex = mutex};
    return rbuf;
}

void ringbuf_put(ringbuf_t *rbuf, float new_item)
{
    BaseType_t mutex_obtained = xSemaphoreTake(rbuf_mutex, portMAX_DELAY);
    if (!mutex_obtained)
    {
        return;
    }
    rbuf_get_idx = (rbuf_get_idx + 1) % rbuf_capacity;
    rbuf_data[rbuf_get_idx] = new_item;
    xSemaphoreGive(rbuf_mutex);
}

size_t ringbuf_get(ringbuf_t *rbuf, float *dst)
{
    BaseType_t mutex_obtained = xSemaphoreTake(rbuf_mutex, portMAX_DELAY);
    if (!mutex_obtained)
    {
        return 0;
    }
    float get_value = rbuf_data[rbuf_get_idx];
    xSemaphoreGive(rbuf_mutex);
    if (isnan(get_value))
    {
        return 0;
    }
    *dst = get_value;
    return 1;
}

size_t ringbuf_getallsorted(ringbuf_t *rbuf, float dst[])
{
    size_t i;
    for (i = 0; i < rbuf_capacity; i++)
    {
        if (isnan(rbuf_data[i]))
        {
            break;
        }
        dst[i] = rbuf_data[i];
    }
    qsort(dst, i, sizeof(float), compare_floats);
    return i;
}

//==================================================================================================
// STATIC FUNCTIONS
//==================================================================================================

static int compare_floats(const void *a, const void *b)
{
    float arg1 = *(const float *)a;
    float arg2 = *(const float *)b;
    if (arg1 < arg2)
    {
        return -1;
    }
    if (arg1 > arg2)
    {
        return 1;
    }
    return 0;
}
