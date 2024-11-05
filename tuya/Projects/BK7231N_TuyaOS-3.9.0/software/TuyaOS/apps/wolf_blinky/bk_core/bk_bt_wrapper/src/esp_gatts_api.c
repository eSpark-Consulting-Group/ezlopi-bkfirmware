#include "esp_gatts_api.h"
#include "esp_log.h"

#include "bt_event_stack.h"
#include "bt_event_type.h"
#include "bk_bt_hal.h"

#define TAG "esp_gatts_api"

/* Saving static callbacks*/

static esp_gatts_cb_t __gatt_reg_callback_s;

esp_err_t esp_ble_gatts_register_callback(esp_gatts_cb_t callback)
{
    ESP_LOGI(TAG, "registering gatts callback");
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
    ESP_LOGI(TAG, "creating service with ID %d", gatts_if);
    bk_bt_service_config_t __service_config = {0};
    __service_config.handle = TKL_BLE_GATT_INVALID_HANDLE;
    __service_config.num_chars = ((num_handle - 1)/2);
    switch (service_id->id.uuid.len)
    {
    case ESP_UUID_LEN_16:
        __service_config.svc_uuid.uuid_type = TKL_BLE_UUID_TYPE_16;
        __service_config.svc_uuid.uuid.uuid16 = service_id->id.uuid.uuid.uuid16;
        break;
    case ESP_UUID_LEN_32:
        __service_config.svc_uuid.uuid_type = TKL_BLE_UUID_TYPE_32;
        __service_config.svc_uuid.uuid.uuid32 = service_id->id.uuid.uuid.uuid32;
        break;
    case ESP_UUID_LEN_128:
        __service_config.svc_uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
        memcpy(__service_config.svc_uuid.uuid.uuid128, service_id->id.uuid.uuid.uuid128, sizeof(__service_config.svc_uuid.uuid.uuid128));
        // __service_config.svc_uuid.uuid.uuid128 = service_id->id.uuid.uuid.uuid128;
        break;
    default:
        break;
    }
    __service_config.type = TKL_BLE_UUID_SERVICE_PRIMARY;
    bk_bt_hal_create_service(&__service_config);
    push((Stack*)esp_bt_get_event_stack_ptr(), (void*)&((esp_bt_event_t){gatts_if, num_handle, "Add Characterstics Event"}));
    printStack((Stack*)esp_bt_get_event_stack_ptr(), &bt_event_print);
    return ESP_OK;
}                                     

esp_err_t esp_ble_gatts_start_service(uint16_t service_handle)
{
    bk_bt_hal_start_services();
    return ESP_OK;
}

esp_err_t esp_ble_gatts_add_char(uint16_t service_handle,  esp_bt_uuid_t  *char_uuid,
                                 esp_gatt_perm_t perm, esp_gatt_char_prop_t property, esp_attr_value_t *char_val,
                                 esp_attr_control_t *control)
{

    bk_bt_char_config_t __characterstic_config = {0};

    __characterstic_config.handle = TKL_BLE_GATT_INVALID_HANDLE;


    switch (char_uuid->len)
    {
    case ESP_UUID_LEN_16:
        __characterstic_config.char_uuid.uuid_type = TKL_BLE_UUID_TYPE_16;
        __characterstic_config.char_uuid.uuid.uuid16 = char_uuid->uuid.uuid16;
        break;
    case ESP_UUID_LEN_32:
        __characterstic_config.char_uuid.uuid_type = TKL_BLE_UUID_TYPE_32;
        __characterstic_config.char_uuid.uuid.uuid32 = char_uuid->uuid.uuid32;
        break;
    case ESP_UUID_LEN_128:
        __characterstic_config.char_uuid.uuid_type = TKL_BLE_UUID_TYPE_128;
        memcpy(__characterstic_config.char_uuid.uuid.uuid128, char_uuid->uuid.uuid128, sizeof(__characterstic_config.char_uuid.uuid.uuid128));
        break;
    default:
        break;
    }

    __characterstic_config.property = property;
    __characterstic_config.permission = perm;
    __characterstic_config.value_len = char_val->attr_len;

    bk_bt_hal_create_characteristic(&__characterstic_config);
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
    if(__gatt_reg_callback_s)
    {
        esp_ble_gatts_cb_param_t gatts_cb_param = {0};
        gatts_cb_param.reg.status = ESP_GATT_INVALID_HANDLE;
        gatts_cb_param.reg.app_id = app_id;

        __gatt_reg_callback_s(ESP_GATTS_REG_EVT, (esp_gatt_if_t)app_id, &(gatts_cb_param));
        {
            gatts_cb_param.add_char.attr_handle =  ESP_GATT_INVALID_HANDLE;
            // __gatt_reg_callback_s(ESP_GATTS_ADD_CHAR_EVT, (esp_gatt_if_t)app_id, &(gatts_cb_param));
        }
    }
    return ESP_OK;
}