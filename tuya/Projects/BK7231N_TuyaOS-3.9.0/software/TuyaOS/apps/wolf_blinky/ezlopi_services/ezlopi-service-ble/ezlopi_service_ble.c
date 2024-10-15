#include "sdkconfig.h"

#ifdef CONFIG_EZPI_BLE_ENABLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "lwip/ip_addr.h"
#include "ezlopi_util_trace.h"

// #include "EZLOPI_USER_CONFIG.h"

#include "ezlopi_core_nvs.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_ble_config.h"

#include "ezlopi_service_ble.h"

extern void ezlopi_ble_service_wifi_provisioning_init(void);
extern void ezlopi_ble_service_device_info_init(void);

static void ezlopi_ble_basic_init(void);

#if (1 == CONFIG_EZPI_BLE_ENALBE_PAIRING)
static void ezlopi_ble_start_secure_gatt_server_open_pairing(void);
#endif

void ezlopi_ble_service_init(void)
{
    TRACE_D("BLE INIT!");
    ezlopi_ble_service_wifi_provisioning_init();
    ezlopi_ble_service_device_info_init();

    ezlopi_ble_basic_init();

    // CHECK_PRINT_ERROR(esp_ble_gatt_set_local_mtu(517), "set local  MTU failed"); //TODO: [SERVICE BLE] implement MTU logic for BEKEN

#if (1 == CONFIG_EZPI_BLE_ENALBE_PAIRING)
    // ezlopi_ble_start_secure_gatt_server_open_pairing(); TODO: [SERVICE BLE] add open pairing after all BLE logic
#endif // 1 == CONFIG_EZPI_BLE_ENALBE_PAIRING
}


#if (1 == CONFIG_EZPI_BLE_ENALBE_PAIRING)
static void ezlopi_ble_start_secure_gatt_server_open_pairing(void) //TODO: [SERVICE BLE] COMPLETE OPEN PAIRING LOGIC
{

}
#endif

static void ezlopi_ble_basic_init(void)
{
    // static const char *ble_device_name = "LED";
    char ble_device_name[32];
    memset(ble_device_name, 0, sizeof(ble_device_name));

    // s_ezlopi_factory_info_t *factory = ezlopi_factory_info_get_info();
    // snprintf(ble_device_name, sizeof(ble_device_name), "ezlopi_%llu", ezlopi_factory_info_v3_get_id());

    const char* device_type = ezlopi_factory_info_v3_get_device_type();

    if ((1 == ezlopi_factory_info_v3_get_provisioning_status()) && (NULL != device_type))
    {
        snprintf(ble_device_name, sizeof(ble_device_name), "%s_%llu", device_type, ezlopi_factory_info_v3_get_id());
    }
    else
    {
        uint8_t mac[6] = {'a', 'b', 'c', 'c', 'd', 'e'};
        // memset(mac, 0, sizeof(mac));
        // esp_read_mac(mac, ESP_MAC_BT);
        snprintf(ble_device_name, sizeof(ble_device_name), "ezlopi_%02x%02x%02x%02x%02x%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // dump("ble_device_name", ble_device_name, 0, sizeof(ble_device_name));

    ezlopi_core_ble_config_init();
}

int ezlopi_ble_service_get_ble_mac(uint8_t mac[6])
{
    int ret = 0x6666; //TODO: [SERVICE BLE] implement MAC logic after BLE logic
    // if (ESP_OK == esp_read_mac(mac, ESP_MAC_BT))
    // {
    //     ret = 1;
    // }
    return ret;
}
#endif // CONFIG_EZPI_BLE_ENABLE


