#ifndef ESP_ERR_H
#define ESP_ERR_H

#include "tal_log.h"

#define ESP_ERROR_CHECK(x) do {                                         \
        esp_err_t err_rc_ = (x);                                        \
        if ((err_rc_ != ESP_OK)) {                              \
            TAL_PR_ERROR(err_rc_, __FILE__, __LINE__,        \
                                    __FUNCTION__, #x);                 \
        }                                                               \
    } while(0)

typedef enum {
    ESP_OK = 0,
    ESP_ERR,
    ESP_FAIL,
    ESP_ERR_NO_MEM,    // Memory allocation failure
    ESP_ERR_INVALID_ARG, // Invalid argument or parsing failure
    ESP_ERR_INVALID_SIZE,
    ESP_ERR_NVS_NOT_FOUND,
    ESP_ERR_NVS_INVALID_LENGTH,
    ESP_ERR_NOT_ALLOWED,
}esp_err_t;

const char* esp_err_to_name(esp_err_t err);

#endif //ESP_ERR_H