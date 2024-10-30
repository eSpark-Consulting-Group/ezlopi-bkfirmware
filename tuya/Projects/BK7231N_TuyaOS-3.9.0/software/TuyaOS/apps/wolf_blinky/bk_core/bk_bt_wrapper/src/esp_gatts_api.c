#include "esp_gatts_api.h"

/* Saving static callbacks*/

esp_gatts_cb_t __gatt_reg_callback_s;

esp_err_t esp_ble_gatts_register_callback(esp_gatts_cb_t callback)
{
    __gatt_reg_callback_s = callback;
    return ESP_OK;
}

esp_gatts_cb_t esp_ble_gatts_get_callback(void)
{
    return (esp_gatts_cb_t)__gatt_reg_callback_s;
}

esp_err_t esp_ble_gatts_send_response(esp_gatt_if_t gatts_if, uint16_t conn_id, uint32_t trans_id,
                                      esp_gatt_status_t status, esp_gatt_rsp_t *rsp)
{
    return ESP_OK;
}                   

esp_err_t esp_ble_gatts_create_service(esp_gatt_if_t gatts_if,
                                       esp_gatt_srvc_id_t *service_id, uint16_t num_handle)
{
    return ESP_OK;
}                                     

esp_err_t esp_ble_gatts_start_service(uint16_t service_handle)
{
    return ESP_OK;
}

esp_err_t esp_ble_gatts_add_char(uint16_t service_handle,  esp_bt_uuid_t  *char_uuid,
                                 esp_gatt_perm_t perm, esp_gatt_char_prop_t property, esp_attr_value_t *char_val,
                                 esp_attr_control_t *control)
{
    return ESP_OK;
}                                 

esp_err_t esp_ble_gatts_add_char_descr (uint16_t service_handle,
                                        esp_bt_uuid_t   *descr_uuid,
                                        esp_gatt_perm_t perm, esp_attr_value_t *char_descr_val,
                                        esp_attr_control_t *control)
{
    return ESP_OK;
}      


esp_err_t esp_ble_gatts_app_register(uint16_t app_id)
{
    return ESP_OK;
}