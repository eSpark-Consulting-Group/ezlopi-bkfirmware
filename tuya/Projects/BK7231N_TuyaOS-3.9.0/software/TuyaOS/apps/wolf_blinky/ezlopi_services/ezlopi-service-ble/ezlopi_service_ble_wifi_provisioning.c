#include "ezlopi_util_trace.h"

#include "ezlopi_core_ble_config.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_cloud_keywords.h"
#include "cJSON.h"

#include "ezlopi_core_factory_info.h"

#include "ezlopi_service_ble_ble_auth.h"
#include "ezlopi_service_ble.h"

void wifi_creds_write_func(char* value, int len);
static void wifi_creds_parse_and_connect(uint8_t* value, uint32_t len);

void wifiwritebar(char* data, int len)
{
    TRACE_D("USELESS FOO CALLED");
}

void ezlopi_ble_service_wifi_provisioning_init(void)
{
    TKL_BLE_UUID_T svc_uuid;
    svc_uuid.uuid_type = TKL_BLE_UUID_TYPE_16;
    svc_uuid.uuid.uuid16 = BLE_PROVISIONING_SERVICE_UUID;
    
    ezlopi_core_ble_gatt_service_create(&svc_uuid);

    svc_uuid.uuid.uuid16 = BLE_PROVISIONING_CHAR_UUID;
    ezlopi_core_ble_gatt_char_create(&svc_uuid, TKL_BLE_GATT_CHAR_PROP_WRITE, TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE, 244);

    svc_uuid.uuid.uuid16 = BLE_WIFI_CHAR_UUID;
    ezlopi_core_ble_gatt_char_create(&svc_uuid, TKL_BLE_GATT_CHAR_PROP_WRITE, TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE, 244);

    svc_uuid.uuid.uuid16 = BLE_RESET_CHAR_UUID;
    TKL_BLE_CHAR_PARAMS_T *ptr_char = ezlopi_core_ble_gatt_char_create(&svc_uuid, TKL_BLE_GATT_CHAR_PROP_WRITE, TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE, 244);

    ezlopi_core_ble_gatt_add_characteristic(ptr_char, 3);

    ezlopi_set_callback(wifi_creds_write_func);
    // ezlopi_set_callback(wifiwritebar);
}

void wifi_creds_write_func(char* value, int len)
{
    TAL_PR_DEBUG("%s called", __FUNCTION__);
    if(value)
    {
        wifi_creds_parse_and_connect(value, len);
    }
    return;
}

static void wifi_creds_parse_and_connect(uint8_t* value, uint32_t len)
{
    TAL_PR_DEBUG("%s called", __FUNCTION__);
    if ((NULL != value) && (len > 0))
    {
        cJSON* root = cJSON_Parse( (const char*)value);

        if (root)
        {
            cJSON* cj_ssid = cJSON_GetObjectItemCaseSensitive(root, ezlopi_wifi_ssid_str);
            cJSON* cj_user_id = cJSON_GetObjectItemCaseSensitive(root, ezlopi_user_id_str);
            cJSON* cj_password = cJSON_GetObjectItemCaseSensitive(root, ezlopi_wifi_password_str);

            if (cj_user_id && cj_user_id->valuestring && cj_password && cj_password->valuestring && cj_ssid && cj_ssid->valuestring)
            {
                char* ssid = cj_ssid->valuestring;
                char* password = cj_password->valuestring;
                char* user_id_str = cj_user_id->valuestring;

                if (user_id_str && ssid && password)
                {
                    // e_auth_status_t l_ble_auth_status = ezlopi_ble_auth_check_user_id(user_id_str);
                    e_auth_status_t l_ble_auth_status = BLE_AUTH_SUCCESS;

                    if (BLE_AUTH_SUCCESS == l_ble_auth_status)
                    {
                        // ezlopi_wifi_connect(ssid, password);
                        // ezlopi_factory_info_v3_set_wifi(ssid, password);
                        TAL_PR_DEBUG("TESTING WIFI BLE CHARACTERSTIC");
                    }
                    else if (BLE_AUTH_USER_ID_NOT_FOUND == l_ble_auth_status)
                    {
                        TAL_PR_DEBUG("TESTING WIFI BLE CHARACTERSTIC 2");
                        // ezlopi_wifi_connect(ssid, password);
                        // ezlopi_factory_info_v3_set_wifi(ssid, password);
                        // ezlopi_ble_auth_store_user_id(user_id_str);
                    }
                }
            }

            cJSON_Delete( root);
        }
    }
}