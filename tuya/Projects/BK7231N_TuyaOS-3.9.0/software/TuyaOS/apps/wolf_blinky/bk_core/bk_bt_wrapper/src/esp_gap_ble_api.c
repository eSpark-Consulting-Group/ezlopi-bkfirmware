#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "tal_ext_bluetooth.h"

static esp_gap_ble_cb_t __gap_reg_callback_s;

static TAL_BLE_DATA_T p_adv_data;
static TAL_BLE_DATA_T p_scan_rsp_data;

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
    //     typedef struct {
    //     uint16_t                adv_int_min;        /*!< Minimum advertising interval for
    //                                                 undirected and low duty cycle directed advertising.
    //                                                 Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
    //                                                 Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    //     uint16_t                adv_int_max;        /*!< Maximum advertising interval for
    //                                                 undirected and low duty cycle directed advertising.
    //                                                 Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
    //                                                 Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec Advertising max interval */
    //     esp_ble_adv_type_t      adv_type;           /*!< Advertising type */
    //     esp_ble_addr_type_t     own_addr_type;      /*!< Owner bluetooth device address type */
    //     esp_bd_addr_t           peer_addr;          /*!< Peer device bluetooth device address */
    //     esp_ble_addr_type_t     peer_addr_type;     /*!< Peer device bluetooth device address type, only support public address type and random address type */
    //     esp_ble_adv_channel_t   channel_map;        /*!< Advertising channel map */
    //     esp_ble_adv_filter_t    adv_filter_policy;  /*!< Advertising filter policy */
    // } esp_ble_adv_params_t;



    ESP_LOGI(TAG, "%s", __FUNCTION__);
    // tal_ext_ble_advertising_data_set(&adv_data, &rsp_data);
    // tal_ext_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
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
                                 