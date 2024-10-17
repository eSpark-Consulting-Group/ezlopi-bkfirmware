// #include <string.h>
#include "sdkconfig.h"
#include "tal_bluetooth_def.h"
#include "tkl_bluetooth_def.h"
#include "tkl_bluetooth.h"
#include "tal_api.h"

#define BLE_WIFI_PROV_SERVICE_INDEX         0
#define BLE_DEVICE_INFO_SERIVCE_INDEX       1

#define BLE_PROV_CHAR_INDEX                 0
#define BLE_WIFI_CHAR_INDEX                 1
#define BLE_RESET_CHAR_INDEX                2
#define BLE_DEVICE_INFO_READ_CHAR_INDEX     3

typedef void (*write_upcall_t)(char* data, int len);

void ezlopi_core_ble_gatt_service_create(TKL_BLE_UUID_T *uuid);
TKL_BLE_CHAR_PARAMS_T* ezlopi_core_ble_gatt_char_create(TKL_BLE_UUID_T *uuid, UCHAR_T _property, UCHAR_T _permission, UCHAR_T _value_len);
void ezlopi_core_ble_gatt_add_characteristic(TKL_BLE_CHAR_PARAMS_T *characterstic, uint8_t num_char);
void ezlopi_core_ble_config_init();
void ezlopi_core_ble_set_device_name(char* ble_device_name);
void ezlopi_set_callback(write_upcall_t cb);
void useless_foo(char* data, int len);

TKL_BLE_CHAR_PARAMS_T* ezlopi_core_ble_get_chr_ptr();
TKL_BLE_SERVICE_PARAMS_T* ezlopi_core_ble_get_svc_ptr();
