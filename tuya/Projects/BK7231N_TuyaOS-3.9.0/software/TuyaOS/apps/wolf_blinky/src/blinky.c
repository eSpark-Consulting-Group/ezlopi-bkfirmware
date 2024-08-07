#include <stdlib.h>
#include "tal_log.h"
#include "uni_log.h"
#include "tkl_gpio.h"
#include "tal_uart.h"
#include "tal_wifi.h"
#include "tkl_output.h"
#include "tal_thread.h"
#include "tal_system.h"
#include "tal_memory.h"

#ifdef ENABLE_WIFI_SERVICE
#include "tuya_iot_wifi_api.h"
#else
#include "tuya_iot_base_api.h"
#endif
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
#include "lwip_init.h"
#endif

#include "ezlopi_core_wifi.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_service_broadcast.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_ezlopi.h"
#include "ezlopi_core_factory_info.h"

#include "esp_partition.h"

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26


/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);
uint32_t __ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
void flash_read_write_test(void);

/* Thread Handles */

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE bk_rcv_thread = NULL;
STATIC THREAD_HANDLE free_heap_thrd = NULL;

void store_requried_data_in_flash()
{
    // #define PROVISION_DATA  "103300323"\
    //                         "20000001"\
    //                         "102uuid7891-815181-iam-prov-uuid1"\
    //                         "9981-i-am-normal8761-uuid"\
    //                         "https://cloud.ezlo.com"\
    //                         "privisioning-token-665"\
    //                         "https://cloud.ezlo.com"\
    //                         "{\"hello\":\"world\"}"
    // #define EZLOPI_CONFIG_JSON "{\"cmd\":3,\"dev_detail\":[{\"dev_type\":1,\"dev_name\":\"switch_temp\",\"id_room\":\"\",\"id_item\":1,\"gpio_in\":24,\"gpio_out\":26,\"pullup_ip\":false,\"pullup_op\":false,\"is_ip\":false,\"ip_inv\":false,\"op_inv\":false,\"val_ip\":false,\"val_op\":false}],\"config_id\":\"1234567\",\"config_time\":1696508363,\"config_name\":\"My moisture sensor\",\"chipset\":\"ESP32\"}"


}

STATIC VOID user_main(VOID_T)
{
        OPERATE_RET rt = OPRT_OK;

    /* LwIP Initialization */
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    TUYA_LwIP_Init();
#endif

    TUYA_CALL_ERR_LOG(tuya_iot_init("./"));

    INT_T result = OPRT_OK;
    TAL_UART_CFG_T cfg = {0};
    cfg.base_cfg.baudrate = 115200;
    cfg.base_cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    cfg.base_cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    cfg.base_cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    cfg.rx_buffer_size = 256;
    cfg.open_mode = O_BLOCK;
    result = tal_uart_init(TUYA_UART_NUM_0, &cfg);
    if (OPRT_OK != result) {
        TAL_PR_ERR("uart init failed", result);
    }

    TAL_PR_DEBUG("hello Ash-----");
    THREAD_CFG_T thread_cfg = 
    {
        .thrdname = "free_heap",
        .stackDepth = 1024 * 3,
        .priority = THREAD_PRIO_5,
    };
    if (NULL == free_heap_thrd) 
    {
        tal_thread_create_and_start(&free_heap_thrd, NULL, NULL, __free_heap_task, NULL, &thread_cfg);
    }

    // ezlopi_wifi_initialize();
    // while (!ezlopi_wifi_is_connected()) 
    // {  
    //     ezlopi_wifi_connect_from_id_bin();
    //     TRACE_E("Not Connected");
    //     tal_system_sleep(700);
    // }
    // TRACE_S("Wifi Connected");

    tal_system_sleep(300);

    ezlopi_init();

    ezlopi_service_broadcast_init();

    ezlopi_service_web_provisioning_init();
    
    return;
}

STATIC VOID tuya_app_thread(VOID_T *arg )
{
    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

VOID_T tuya_app_main(VOID)
{
    THREAD_CFG_T thrd_param = {4096*2, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
STATIC VOID __free_heap_task(PVOID_T args)
{
    int free_heap = 0;
    float free_heap_kb;
    for (;;) {
        free_heap = tal_system_get_free_heap_size();
        free_heap_kb = free_heap/1024;
        // TAL_PR_NOTICE("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        TRACE_W("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        if(free_heap <= 5*1024)
        {
            tal_system_reset();
        }
        tal_system_sleep(3000);
    }
}

uint32_t ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
#if 0
void flash_read_write_test()
{
    // Initialize the flash hardware
    
    hal_flash_init();
    int data_offset = __ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA);
    
    // Prepare the input buffer and its length
    char* inBuffer = " Flash Hello World World Write !";
    int inBufferLen = strlen(inBuffer);

    // Ensure the output buffer is large enough
    char outBuffer[256] = {0};

    TRACE_S("Before -----> %s", outBuffer);

    // // Erase the flash sector before writing
    // if (bk_flash_erase(BK_PARTITION_PROV_DATA, data_offset, inBufferLen) != 0) {
    //     TRACE_E("Flash Erase Failed");
    //     return;
    // }

    // Write to flash
    // if (bk_flash_write(BK_PARTITION_PROV_DATA, data_offset, inBuffer, inBufferLen) == 0) {
    //     TRACE_I("Flash Write Success");
    // } else {
    //     TRACE_E("Flash Write Failed");
    //     return;
    // }

    // Read from flash
    if (bk_flash_read(BK_PARTITION_PROV_DATA, EZLOPI_FINFO_OFFSET_HUB_DATA + EZLOPI_FINFO_LEN_WIFI_SSID, outBuffer, 96) == 0) {
        TRACE_I("Flash Read Success");
    } else {
        TRACE_E("Flash Read Failed");
        return;
    }

    TRACE_S("After -----> %s", outBuffer);
}
#endif

#if 0
void flash_read_write_test()
{
    char* inBuffer = "Hello ESP BK Partition!";
    char outBuffer[256] = "Garbage Value Garbage Value !";
    int inBufferLen = strlen(inBuffer);
    
    esp_partition_t* partition_ctx_v3 = esp_partition_find_first(ESP_PARTITION_PROV_DATA, EZLOPI_FACTORY_INFO_V3_PARTITION_NAME);

    if(NULL == partition_ctx_v3)
    {
        TRACE_E("Partition CTX is NULL");
    }
    TRACE_W("Before Write Output Buffer = %s", outBuffer);
    esp_partition_write(partition_ctx_v3, 0x0, inBuffer, inBufferLen);
    esp_partition_read(partition_ctx_v3, 0x0, outBuffer, inBufferLen);
    TRACE_W("After Write Output Buffer = %s", outBuffer);
    
}
#endif

#if 0
uint32_t __ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type)
{
    if (partition_type == E_EZLOPI_FACTORY_INFO_CONN_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_CONN_DATA);
    }
    else if (partition_type == E_EZLOPI_FACTORY_INFO_HUB_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_HUB_DATA);
    }
    else
    {
        return 0;
    }
}
#endif