#include <stdlib.h>
#include "tal_log.h"
#include "tal_uart.h"
#include "tkl_output.h"
#include "tal_thread.h"
#include "tal_system.h"

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
#include "ezlopi_core_log.h"
#include "ezlopi_service_loop.h"
#include "ezlopi_core_nvs.h"
#include "ezlopi_core_event_group.h"

#include "FreeRTOS.h"
#include "task.h"

/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);

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
        .stackDepth = 2048 * 3,
        .priority = THREAD_PRIO_0,
    };
    if (NULL == free_heap_thrd) 
    {
        tal_thread_create_and_start(&free_heap_thrd, NULL, NULL, __free_heap_task, NULL, &thread_cfg);
    }

    ezlopi_core_set_log_upcalls();
    ezlopi_init();
    ezlopi_ble_service_init();
    ezlopi_service_broadcast_init();
    ezlopi_service_web_provisioning_init();
    ezlopi_nvs_set_boot_count(ezlopi_system_info_get_boot_count() + 1);

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
    THREAD_CFG_T thrd_param = {4096*4, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}

STATIC VOID __free_heap_task(PVOID_T args)
{
    int free_heap = 0;
    float free_heap_kb;

    uint32_t last_tick_count = 0;
    for (;;) {
        free_heap = tal_system_get_free_heap_size();
        free_heap_kb = free_heap/1024.0;
        // TAL_PR_NOTICE("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        TRACE_I("----------------------------------------------");
        TRACE_W("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        TRACE_I("----------------------------------------------");
        if(free_heap <= 5*1024)
        {
            tal_system_reset();
        }
         
        uint32_t tick_count_ms = xTaskGetTickCount() / portTICK_PERIOD_MS;
        // TRACE_I("TICK COUNT = %d, TICK DIFF = %d", tick_count_ms, tick_count_ms - last_tick_count);
        last_tick_count = tick_count_ms;
        if(ezlopi_event_group_wait_for_event(EZLOPI_EVENT_WS_CONNECT, 0 ,false) && !ezlopi_event_group_wait_for_event(EZLOPI_EVENT_LOOP_INIT, 0 ,false))
        {
            ezlopi_service_loop_init();
            ezlopi_event_group_set_event(EZLOPI_EVENT_LOOP_INIT);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    tal_thread_delete(free_heap_thrd);
}