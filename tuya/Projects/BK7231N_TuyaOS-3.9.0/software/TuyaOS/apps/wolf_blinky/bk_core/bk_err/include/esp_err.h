#ifndef ESP_ERR_H
#define ESP_ERR_H

typedef enum {
    ESP_OK = 0,
    ESP_ERR,
    ESP_ERR_NO_MEM,    // Memory allocation failure
    ESP_ERR_INVALID_ARG, // Invalid argument or parsing failure
    ESP_ERR_INVALID_SIZE,
    ESP_ERR_NOT_ALLOWED,
}esp_err_t;


#endif //ESP_ERR_H