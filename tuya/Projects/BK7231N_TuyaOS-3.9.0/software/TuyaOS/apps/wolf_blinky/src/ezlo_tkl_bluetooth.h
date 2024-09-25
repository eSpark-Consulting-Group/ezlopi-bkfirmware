/*
function needed 
tkl_ble_gattc_write_without_rsp
tkl_ble_gattc_char_desc_discovery
tkl_ble_gattc_all_char_discovery
tkl_ble_gattc_write
tkl_ble_gap_connect
ezlo_tkl_ble_gattc_all_service_discovery

converted to 
ezlo_tkl_ble_gattc_write_without_rsp
ezlo_tkl_ble_gattc_char_desc_discovery
ezlo_tkl_ble_gattc_all_char_discovery
ezlo_tkl_ble_gattc_write
ezlo_tkl_ble_gap_connect
ezlo_ezlo_tkl_ble_gattc_all_service_discovery
*/

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "tkl_bluetooth_def.h"

OPERATE_RET ezlo_tkl_ble_gattc_write_without_rsp(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data,
                                            uint16_t length);

OPERATE_RET ezlo_tkl_ble_gattc_all_char_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);

OPERATE_RET ezlo_tkl_ble_gattc_all_service_discovery(uint16_t conn_handle);

OPERATE_RET ezlo_tkl_ble_gatt_callback_register(const TKL_BLE_GATT_EVT_FUNC_CB gatt_evt);
OPERATE_RET ezlo_tkl_ble_gap_callback_register(const TKL_BLE_GAP_EVT_FUNC_CB gap_evt);