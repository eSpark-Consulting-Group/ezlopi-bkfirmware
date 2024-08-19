#include "ezlopi_core_ezlopi.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_buffer.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_sntp.h"

#if defined(DEVICE_PROVISION_BOOT)
#include "cert.h"
void __prov_device();
#endif

static void ezlopi_initialize_devices_v3(void);
void ezlopi_init()
{
    #define CONFIG_EZPI_CORE_STATIC_BUFFER_SIZE 5*1024 //FIXME_AUTO
    ezlopi_core_buffer_init(CONFIG_EZPI_CORE_STATIC_BUFFER_SIZE); // allocate 10kB
    ezlopi_factory_info_v3_init();

    #ifdef DEVICE_PROVISION_BOOT
    __prov_device();
    #endif
    print_factory_info_v3();
    ezlopi_device_prepare();
    ezlopi_initialize_devices_v3();
    EZPI_CORE_sntp_init();
}

l_ezlopi_device_t* link_next_parent_id(uint32_t target_to_clear_parent_id)
{
    l_ezlopi_device_t* pre_devs = ezlopi_device_get_head();
    while (pre_devs)
    {
        if ((NULL != pre_devs->next) &&
            pre_devs->next->cloud_properties.parent_device_id == 0 &&
            pre_devs->next->cloud_properties.device_id > target_to_clear_parent_id)
        {
            return (pre_devs->next);
        }
        pre_devs = pre_devs->next;
    }
    return NULL;
}

static void ezlopi_initialize_devices_v3(void)
{
    int device_init_ret = 0;
    l_ezlopi_device_t* curr_device = ezlopi_device_get_head();

    while (curr_device)
    {

        TRACE_S("Device_id_curr_device : [0x%x] ", curr_device->cloud_properties.device_id);
        l_ezlopi_item_t* curr_item = curr_device->items;
        while (curr_item)
        {
            if (curr_item->func)
            {
                if ((device_init_ret = curr_item->func(EZLOPI_ACTION_INITIALIZE, curr_item, NULL, NULL)) < 0)
                {
                    break;
                }
            }
            else
            {
                TRACE_E("Function is not defined!");
            }
            curr_item = curr_item->next;
        }
        // TRACE_D("ret = %d", device_init_ret);
        if (0 > device_init_ret)
        {
            device_init_ret = 0;
            l_ezlopi_device_t* device_to_free = curr_device;

            if (NULL != curr_device->next &&
                curr_device->cloud_properties.parent_device_id == 0 &&
                curr_device->cloud_properties.device_id == curr_device->next->cloud_properties.parent_device_id)
            {
                /* if 'device_to_free' is parent_with_child_nodes */
                curr_device = link_next_parent_id(curr_device->cloud_properties.device_id);
            }
            else
            {
                curr_device = curr_device->next;
            }

            ezlopi_device_free_device(device_to_free);
        }
        else
        {
            curr_device = curr_device->next;
        }
    }
}

#if defined(DEVICE_PROVISION_BOOT)
void __prov_device()
{
    char* tmp_str = (char*)malloc(sizeof(char)* EZLOPI_FINFO_READ_LEN_4KB);
    #define EZLOPI_CONFIG_JSON "{\"cmd\":3,\"dev_detail\":[{\"dev_type\":1,\"dev_name\":\"switch_temp\",\"id_room\":\"\",\"id_item\":1,\"gpio_in\":24,\"gpio_out\":26,\"pullup_ip\":false,\"pullup_op\":false,\"is_ip\":false,\"ip_inv\":false,\"op_inv\":false,\"val_ip\":false,\"val_op\":false}],\"config_id\":\"1234567\",\"config_time\":1696508363,\"config_name\":\"My ezlo sensor\",\"chipset\":\"ESP32\"}"
    
    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    strcpy(tmp_str, EZLOPI_CONFIG_JSON);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    strcpy(tmp_str, CA_CERT);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    strcpy(tmp_str, SHARED_KEY);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    strcpy(tmp_str, SSL_PRIVATE_KEY);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_PRIVATE_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    /* Conn data provision 4kb */

    int16_t serial_number = 7777;
    int version = 101;
    int build = 3333;
    unsigned long long device_id = 102002871;
    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_SERIAL_NUMBER, &serial_number, EZLOPI_FINFO_LEN_CONFIG_VERSION);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_FMW_VERSION, &version, EZLOPI_FINFO_LEN_VERSION);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_FMW_BUILD, &build, EZLOPI_FINFO_LEN_CONFIG_VERSION);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_DEVICE_ID, &device_id, EZLOPI_FINFO_LEN_DEVICE_ID);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, "eSpark Wifi", EZLOPI_FINFO_LEN_WIFI_SSID);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_WIFI_PASS, "47356880", EZLOPI_FINFO_LEN_WIFI_PASS);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_DEVICE_MAC, "A8:80:55:0E:3D:9B", EZLOPI_FINFO_LEN_DEVICE_MAC);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_DEVICE_NAME, "Ezlo beken", EZLOPI_FINFO_LEN_DEVICE_NAME);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_MANUF_NAME, "Espark Man", EZLOPI_FINFO_LEN_MANUF_NAME);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_BRAND_NAME, "Espark Brand", EZLOPI_FINFO_LEN_BRAND_NAME);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_MODEL_NAME, "Espark Model", EZLOPI_FINFO_LEN_MODEL_NAME);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SERIAL_NUMBER, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    /* Hub data provision without ssl and config string just first 4kb */

    int16_t config_version = 1;
    char* prov_uuid = "b9c0e91d-7614-48b7-b9cd-09eef78200b9";
    char* uuid = "98c9faf2-2a91-46b4-aebb-11befd92e4c4";
    char* prov_token = "pVmAKypMR1ggr50qRn4DFEdiLkBhxfFQgydrTwIpenFrJTfdeqmnN4A9rsAdATvLdbdgCrGFzQFJVvH5T1aI49f8DHYG6asAxHL2DPtpGoSpoJwO08ONqI6Pn5de6W9oabwZqnxGQNFIgwlQSwXTiHmJNU9mqFVijqHugwYJFN9uLt4D01reLY1ZZLHK2dPa3GT4Ol3V4gouKrpithhzxcn8sqlvou31DHMN8SMpOzRTpHRCmtb589c9oyfV8OQG";
    char* cloud_server = "https://cloud.ezlo.com/getserver";
    
    memset(tmp_str, 0,  EZLOPI_FINFO_READ_LEN_4KB);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_CONFIG_VERSION, &config_version, EZLOPI_FINFO_LEN_CONFIG_VERSION);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_UUID, prov_uuid, EZLOPI_FINFO_LEN_PROV_UUID);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_DEVICE_UUID, uuid, EZLOPI_FINFO_LEN_DEVICE_UUID);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_TOKEN, prov_token, EZLOPI_FINFO_LEN_PROVISIONING_TOKEN);
    memcpy(tmp_str + EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, cloud_server, EZLOPI_FINFO_LEN_CLOUD_SERVER_URL);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(0x0, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_READ_LEN_4KB);

    free(tmp_str);
}
#endif