#include "ezlopi_core_ble_config.h"
#include "ezlopi_service_ble.h"

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
}