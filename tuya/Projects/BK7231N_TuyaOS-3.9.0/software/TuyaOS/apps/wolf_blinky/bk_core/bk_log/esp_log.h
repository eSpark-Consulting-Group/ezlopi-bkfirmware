#include "tal_log.h"
#define TAG "esp_gap_ble_api"

#define ESP_LOGI(x, y,...) TAL_PR_INFO(x" "y, ##__VA_ARGS__)
#define ESP_LOGD(x, y,...) TAL_PR_DEBUG(x" "y, ##__VA_ARGS__)
#define ESP_LOGE(x, y,...) TAL_PR_ERR(x" "y, ##__VA_ARGS__)