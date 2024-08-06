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
#include "BkDriverFlash.h"

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

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26


/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);
void flash_read_write_test(void);

/* Thread Handles */

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE bk_rcv_thread = NULL;
STATIC THREAD_HANDLE free_heap_thrd = NULL;

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

    ezlopi_wifi_initialize();
    while (!ezlopi_wifi_is_connected()) 
    {  
        ezlopi_wifi_connect_from_id_bin();
        TRACE_E("Not Connected");
        tal_system_sleep(200);
    }
    TRACE_S("Wifi Connected");

    ezlopi_init();

    ezlopi_service_broadcast_init();

    tal_system_sleep(1000);

    ezlopi_service_web_provisioning_init();
    
    flash_read_write_test();

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

void flash_read_write_test()
{
    // Initialize the flash hardware
    
    hal_flash_init();
    int data_offset = 0x0;
    
    // Prepare the input buffer and its length
    char* inBuffer = "Hello Flash Write World!";
    int inBufferLen = strlen(inBuffer);

    // Ensure the output buffer is large enough
    char outBuffer[256] = {0};

    TRACE_S("Before -----> %s", outBuffer);

    // Erase the flash sector before writing
    if (bk_flash_erase(BK_PARTITION_NET_PARAM, data_offset, inBufferLen) != 0) {
        TRACE_E("Flash Erase Failed");
        return;
    }

    // Write to flash
    if (bk_flash_write(BK_PARTITION_NET_PARAM, data_offset, inBuffer, inBufferLen) == 0) {
        TRACE_I("Flash Write Success");
    } else {
        TRACE_E("Flash Write Failed");
        return;
    }

    // Read from flash
    if (bk_flash_read(BK_PARTITION_NET_PARAM, data_offset, outBuffer, inBufferLen) == 0) {
        TRACE_I("Flash Read Success");
    } else {
        TRACE_E("Flash Read Failed");
        return;
    }

    TRACE_S("After -----> %s", outBuffer);
}