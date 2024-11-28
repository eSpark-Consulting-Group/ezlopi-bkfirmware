#include "esp_system.h"
#include "tal_wifi.h"

esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t type)
{
    OPERATE_RET op_ret = tal_wifi_get_mac(WF_AP , mac);
    if(OPRT_OK != op_ret)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}