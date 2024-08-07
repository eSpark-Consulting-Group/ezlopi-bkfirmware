#include "ezlopi_core_ezlopi.h"
#include "ezlopi_core_devices.h"
#include "ezlopi_core_buffer.h"
#include "ezlopi_core_factory_info.h"

#include "cert.h"

static void ezlopi_initialize_devices_v3(void);
void __prov_device();
void ezlopi_init()
{
    #define CONFIG_EZPI_CORE_STATIC_BUFFER_SIZE 5*1024 //FIXME_AUTO
    ezlopi_core_buffer_init(CONFIG_EZPI_CORE_STATIC_BUFFER_SIZE); // allocate 10kB
    ezlopi_factory_info_v3_init();

    #define DEVICE_PROVISION_BOOT
    #ifdef DEVICE_PROVISION_BOOT
    __prov_device();
    #endif
    print_factory_info_v3();
    ezlopi_device_prepare();
    ezlopi_initialize_devices_v3();
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

void __prov_device()
{
    char* tmp_str = (char*)malloc(sizeof(char)*1024);
    #define EZLOPI_CONFIG_JSON "{\"cmd\":3,\"dev_detail\":[{\"dev_type\":1,\"dev_name\":\"switch_temp\",\"id_room\":\"\",\"id_item\":1,\"gpio_in\":24,\"gpio_out\":26,\"pullup_ip\":false,\"pullup_op\":false,\"is_ip\":false,\"ip_inv\":false,\"op_inv\":false,\"val_ip\":false,\"val_op\":false}],\"config_id\":\"1234567\",\"config_time\":1696508363,\"config_name\":\"My ezlo sensor\",\"chipset\":\"ESP32\"}"
    
    memset(tmp_str, 0, 1024);
    strcpy(tmp_str, EZLOPI_CONFIG_JSON);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA), strlen(tmp_str));

    memset(tmp_str, 0, 1024);
    strcpy(tmp_str, CA_CERT);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, E_EZLOPI_FACTORY_INFO_CONN_DATA), strlen(tmp_str));

    memset(tmp_str, 0, 1024);
    strcpy(tmp_str, SHARED_KEY);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), strlen(tmp_str));

    memset(tmp_str, 0, 1024);
    strcpy(tmp_str, SSL_PRIVATE_KEY);
    pub_ezlopi_factory_info_v3_set_4kb(tmp_str, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_PRIVATE_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), strlen(tmp_str));
}