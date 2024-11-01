#include "tal_ext_bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

// Structure for service configuration
typedef struct {
    USHORT_T handle;                  /**< After init, we will get the svc-handle */
    TKL_BLE_UUID_T svc_uuid;          /**< Service UUID */
    TKL_BLE_SERVICE_TYPE_E type;      /**< Service Type */
} bk_bt_service_config_t;

// Structure for characteristic configuration
typedef struct {
    USHORT_T handle;                   /**< Output: char-handle to be stored */
    TKL_BLE_UUID_T char_uuid;          /**< Characteristics UUID */
    UCHAR_T property;                  /**< Characteristics property */
    UCHAR_T permission;                /**< Characteristics value attribute permission */
    UCHAR_T value_len;                 /**< Characteristics value length */
} bk_bt_char_config_t;

void bk_bt_hal_create_service(const bk_bt_service_config_t* config);
void bk_bt_hal_create_characteristic(const bk_bt_char_config_t* config);

#ifdef __cplusplus
}
#endif
