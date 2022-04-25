/*
 * This header file provides macros for shortening error-handling.
 * It expects the preprocessor constant `ESP_LOG_TAG` to be defined before the header file is included.
 *
 * Example:
 * ```c
 * #define ESP_LOG_TAG "BTDM_INIT"
 * #include "iferr.h"
 *
 * int main(void)
 * {
 *     IFERR_LOG(some_fn(), "failed to parse input");
 * }
 * ```
 */

#ifdef ESP_LOG_TAG

#include "esp_err.h"

/*
 * If error, log error and continue execution
 */
#define IFERR_LOG(x, format, args...)                                                                                  \
    ({                                                                                                                 \
        esp_err_t err_rc_ = (x);                                                                                       \
        if (err_rc_ != ESP_OK)                                                                                         \
        {                                                                                                              \
            ESP_LOGE(ESP_LOG_TAG, "%s - " format ", error = %s", __func__, ##args, esp_err_to_name(err_rc_));          \
        }                                                                                                              \
    })

/*
 * If error, log error and return void
 */
#define IFERR_RETV(x, format, args...)                                                                                 \
    ({                                                                                                                 \
        esp_err_t err_rc_ = (x);                                                                                       \
        if (err_rc_ != ESP_OK)                                                                                         \
        {                                                                                                              \
            ESP_LOGE(ESP_LOG_TAG, "%s - " format ", error = %s", __func__, ##args, esp_err_to_name(err_rc_));          \
            return;                                                                                                    \
        }                                                                                                              \
    })

/*
 * If error, log error and return error
 */
#define IFERR_RETE(x, format, args...)                                                                                 \
    ({                                                                                                                 \
        esp_err_t err_rc_ = (x);                                                                                       \
        if (err_rc_ != ESP_OK)                                                                                         \
        {                                                                                                              \
            ESP_LOGE(ESP_LOG_TAG, "%s - " format ", error = %s", __func__, ##args, esp_err_to_name(err_rc_));          \
            return err_rc_;                                                                                            \
        }                                                                                                              \
    })

#endif // ESP_LOG_TAG
