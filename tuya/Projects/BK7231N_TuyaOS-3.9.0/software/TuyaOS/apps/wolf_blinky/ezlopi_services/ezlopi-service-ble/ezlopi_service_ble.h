
#ifndef _EZLOPI_SERVICE_BLE_H_
#define _EZLOPI_SERVICE_BLE_H_

#ifdef CONFIG_EZPI_BLE_ENABLE

#define CHECK_PRINT_ERROR(x, msg)                                    \
    {                                                                \
        if (x)                                                       \
        {                                                            \
            TRACE_E("%s %s: %s", __func__, msg, esp_err_to_name(x)); \
            return;                                                  \
        }                                                            \
    }

#define BLE_PROVISIONING_SERVICE_UUID 0x00E3
#define BLE_PROVISIONING_CHAR_UUID 0xE301
#define BLE_WIFI_CHAR_UUID 0xE302
#define BLE_RESET_CHAR_UUID 0xE303

#define BLE_DEVICE_INFO_SERVICE_UUID 0x00E4
#define BLE_DEVICE_INFO_CHAR_UUID 0xE401

void ezlopi_ble_service_init(void);
int ezlopi_ble_service_get_ble_mac(uint8_t mac[6]);

#else// CONFIG_EZPI_BLE_ENABLE
    #warning "CONFIG_EZPI_BLE_ENABLE is not defined!"
#endif

#endif //  _EZLOPI_SERVICE_BLE_H_
