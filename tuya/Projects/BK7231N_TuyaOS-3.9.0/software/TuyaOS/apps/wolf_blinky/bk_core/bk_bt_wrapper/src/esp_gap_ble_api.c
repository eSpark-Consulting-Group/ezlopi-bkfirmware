#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "tal_ext_bluetooth.h"

static esp_gap_ble_cb_t __gap_reg_callback_s;

STATIC UCHAR_T adv_data_const[31] =
{
    0x02,
    0x01,
    0x06,
    0x03,
    0x02,
    0xF0, 0x05,
    0x17,
    0x16,
    0xF0, 0x05,
    0x41, 0x00,       //Frame Control
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC UCHAR_T scan_rsp_data_const[31] =
{   
	0x1E, //24
    0x09,
    'e', 'z', 'l', 'o', 'p', 'i', '_', 'g', 'e', 'n', 'e', 'r', 'i', 'c', '_', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X',
    // '_', 'b', 'k', '7', '2'
};

esp_err_t esp_ble_gap_register_callback(esp_gap_ble_cb_t callback)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    __gap_reg_callback_s = callback;
    return ESP_OK;
}

esp_gap_ble_cb_t esp_ble_gap_get_callback(void)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return (esp_gap_ble_cb_t)__gap_reg_callback_s;
}

esp_err_t esp_ble_gap_config_adv_data (esp_ble_adv_data_t *adv_data)
{   
    //TODO: Somehow extract the adv_data and place it in bk p_adv_data
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t esp_ble_gap_set_scan_params(esp_ble_scan_params_t *scan_params)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t esp_ble_gap_start_advertising (esp_ble_adv_params_t *adv_params)
{

    TAL_BLE_DATA_T adv_data;
    TAL_BLE_DATA_T rsp_data;

    adv_data.p_data = adv_data_const;
    adv_data.len = sizeof(adv_data_const);
    rsp_data.p_data = scan_rsp_data_const;
    rsp_data.len = sizeof(scan_rsp_data_const);


    ESP_LOGI(TAG, "%s", __FUNCTION__);
    tal_ext_ble_advertising_data_set(&adv_data, &rsp_data);
    tal_ext_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
    return ESP_OK;
}

esp_err_t esp_ble_set_advertising (esp_ble_adv_params_t *adv_params)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t esp_ble_gap_update_conn_params(esp_ble_conn_update_params_t *params)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t esp_ble_gap_stop_advertising(void)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

#define MAX_SCAN_RSP_DATA_LEN 31

void update_ble_adv_name(UCHAR_T *src, const char *new_name) {
    // Find the name field (AD type 0x09) in the scan response data
    for (int i = 0; i < MAX_SCAN_RSP_DATA_LEN; ) {
        UCHAR_T length = src[i];
        if (length == 0 || i + length >= MAX_SCAN_RSP_DATA_LEN) {
            break; // End of valid data
        }

        if (src[i + 1] == 0x09) { // Check if the AD type is "Complete Local Name" (0x09)
            // Update the name
            size_t name_len = length - 1; // Length minus AD type byte
            size_t new_name_len = strlen(new_name);

            // Ensure the new name fits within the length field
            if (new_name_len > name_len) {
                new_name_len = name_len;
            }

            memcpy(&src[i + 2], new_name, new_name_len);

            // Pad with zeros if the new name is shorter
            if (new_name_len < name_len) {
                memset(&src[i + 2 + new_name_len], 0, name_len - new_name_len);
            }

            break; // Name updated, exit loop
        }

        i += length + 1; // Move to the next AD structure
    }
}

esp_err_t esp_ble_gap_set_device_name(const char *name)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    esp_err_t ret = ESP_FAIL;

    update_ble_adv_name(scan_rsp_data_const, name);

    return ret;
}
                                 