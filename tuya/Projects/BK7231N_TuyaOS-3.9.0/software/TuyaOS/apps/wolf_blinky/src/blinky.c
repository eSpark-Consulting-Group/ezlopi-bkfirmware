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
#include "ezlopi_util_trace.h"

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26


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

    TUYA_GPIO_NUM_E pin_id = LED_PIN;
    TUYA_CALL_ERR_LOG(tuya_iot_init("./"));
    TUYA_GPIO_BASE_CFG_T led_cfg = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_LOW
    };
    TUYA_CALL_ERR_LOG(tkl_gpio_init(pin_id, &led_cfg));
    UINT32_T blink_interval = 1000;

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
        .stackDepth = 1024,
        .priority = THREAD_PRIO_5,
    };
    if (NULL == free_heap_thrd) 
    {
        tal_thread_create_and_start(&free_heap_thrd, NULL, NULL, __free_heap_task, NULL, &thread_cfg);
    }
    sys_msleep(4000);

    ezlopi_wifi_initialize();
    while (!ezlopi_wifi_is_connected()) 
    {  
        ezlopi_wifi_connect_from_id_bin();
        TRACE_E("Not Connected");
        tal_system_sleep(1000);
    }
    TRACE_S("Wifi Connected");

    ezlopi_service_web_provisioning_init();
    
    while(1) {  
        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_HIGH);
        // tal_uart_write(TUYA_UART_NUM_0, turnOn, sizeof(turnOn));
        tal_system_sleep(blink_interval);

        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_LOW);
        tal_log_set_manage_attr(TAL_LOG_LEVEL_DEBUG);
        // tal_uart_write(TUYA_UART_NUM_0, turnOff, sizeof(turnOff));
        tal_system_sleep(blink_interval);
        // TRACE_E("CHECKING TRACEEEEEE ");
        // TRACE_D("CHECKING TRACEEEEEE ");
        // TRACE_W("CHECKING TRACEEEEEE ");
        // TRACE_I("CHECKING TRACEEEEEE ");
        // TRACE_S("CHECKING TRACEEEEEE ");

    }

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
    for (;;) {
        TAL_PR_NOTICE("free heap: %d", tal_system_get_free_heap_size());
        tal_system_sleep(3000);
    }
}
