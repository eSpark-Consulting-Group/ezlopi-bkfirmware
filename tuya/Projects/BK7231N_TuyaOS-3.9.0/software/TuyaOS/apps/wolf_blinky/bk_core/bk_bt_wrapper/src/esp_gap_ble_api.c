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
    0x50, 0xFD,
    0x17,
    0x16,
    0x50, 0xFD,
    0x41, 0x00,       //Frame Control
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC UCHAR_T scan_rsp_data_const[31] =
{   
    0x17,             // length
    0xFF,
    0xD0,
    0x07,
    0x00, //Encry Mode(8)
    0x00,0x00, //communication way bit0-mesh bit1-wifi bit2-zigbee bit3-NB
    0x00, //FLAG
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x05, //24
    0x09,
    0x4D, 0x61, 0x61, 0x7a
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
                                 