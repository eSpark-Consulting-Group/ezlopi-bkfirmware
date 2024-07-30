#ifndef ESP_ERR_H
#define ESP_ERR_H

typedef enum {
    ESP_OK,
    ESP_ERR,
    ESP_ERR_NO_MEM,    // Memory allocation failure
    ESP_ERR_INVALID_ARG // Invalid argument or parsing failure
}esp_err_t;


#endif //ESP_ERR_H