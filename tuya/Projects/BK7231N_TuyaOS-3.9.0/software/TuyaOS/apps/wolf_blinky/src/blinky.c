#include <stdlib.h>
#include "tal_log.h"
#include "tal_uart.h"
#include "tkl_output.h"
#include "tal_thread.h"
#include "tal_system.h"

#include "tal_ext_bluetooth.h"

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
#include "ezlopi_service_ble.h"
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

STATIC UCHAR_T adv_data_const[31] =
{
    0x02,
    0x01,
    0x06,
    0x03,
    0x02,
    0x50, 0xFD,
    0x17,
    0x16,
    0x50, 0xFD,
    0x41, 0x00,       //Frame Control
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC UCHAR_T scan_rsp_data_const[31] =
{   
    0x17,             // length
    0xFF,
    0xD0,
    0x07,
    0x00, //Encry Mode(8)
    0x00,0x00, //communication way bit0-mesh bit1-wifi bit2-zigbee bit3-NB
    0x00, //FLAG
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x05, //24
    0x09,
    0x4D, 0x61, 0x61, 0x7a
};

/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);

/* Thread Handles */

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE bk_rcv_thread = NULL;
STATIC THREAD_HANDLE free_heap_thrd = NULL;
STATIC char ble_rx_buffer[2 * 1024] = {0};
STATIC TAL_BLE_PEER_INFO_T sg_ble_peripheral_info;


STATIC VOID_T ___bal_ble_peripheral_event_callback(TAL_BLE_EVT_PARAMS_T *p_event)
{
    TAL_PR_DEBUG("----------ble_peripheral event callback-------");
    TAL_PR_DEBUG("ble_peripheral event is : %d", p_event->type);
    switch(p_event->type) {
        case TAL_BLE_STACK_INIT: {
            TAL_PR_DEBUG("init Ble/Bt stack and start advertising.");
            if(p_event->ble_event.init == 0) {
                TAL_BLE_DATA_T adv_data;
                TAL_BLE_DATA_T rsp_data;

                adv_data.p_data = adv_data_const;
                adv_data.len = sizeof(adv_data_const);
                rsp_data.p_data = scan_rsp_data_const;
                rsp_data.len = sizeof(scan_rsp_data_const);
                
                tal_system_sleep(1000);
                tal_ext_ble_advertising_data_set(&adv_data, &rsp_data);
                tal_ext_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
            }
            break;
        }
        case TAL_BLE_EVT_PERIPHERAL_CONNECT: {
            TAL_PR_DEBUG("ble_peripheral starts to connect...");
            if(p_event->ble_event.connect.result == 0) {
                TAL_BLE_DATA_T read_data;
                UCHAR_T read_buffer[512];

                memcpy(&sg_ble_peripheral_info, &p_event->ble_event.connect.peer, sizeof(TAL_BLE_PEER_INFO_T));
                memcpy(read_buffer, adv_data_const, sizeof(adv_data_const));
                memcpy(&read_buffer[sizeof(adv_data_const)], scan_rsp_data_const, sizeof(scan_rsp_data_const));

                read_data.p_data = read_buffer;
                read_data.len = sizeof(adv_data_const) + sizeof(scan_rsp_data_const);

                // Verify Read Char
                tal_ext_ble_server_common_read_update(&read_data);
            } else {
                memset(&sg_ble_peripheral_info, 0, sizeof(TAL_BLE_PEER_INFO_T));
            }
            break;
        }
        case TAL_BLE_EVT_DISCONNECT: {
            TAL_PR_DEBUG("ble_peripheral disconnect.");
            tal_ext_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
            break;
        }
        case TAL_BLE_EVT_CONN_PARAM_UPDATE: {
            TAL_PR_DEBUG("Parameter update successfully!");
            // Show Connect Parameters Info
            TAL_PR_DEBUG("Conn Param Update: Min = %f ms, Max = %f ms, Latency = %d, Sup = %d ms", 
                p_event->ble_event.conn_param.conn.min_conn_interval * 1.25, \
                p_event->ble_event.conn_param.conn.max_conn_interval * 1.25,\
                p_event->ble_event.conn_param.conn.latency, \
                p_event->ble_event.conn_param.conn.conn_sup_timeout * 10);
            break;  
        } 
        case TAL_BLE_EVT_MTU_REQUEST: {
            TAL_PR_DEBUG("MTU exchange request event.");
            TAL_PR_DEBUG("Get Response MTU Size = %d", p_event->ble_event.exchange_mtu.mtu);
            break;
        }
        case TAL_BLE_EVT_WRITE_REQ: {
            INT_T i = 0;
            TAL_PR_DEBUG("Get Device-Write Char Request");
            for ( i = 0; i < p_event->ble_event.write_report.report.len; i++) {
                TAL_PR_DEBUG("devicr send  data[%d]: %d", i, p_event->ble_event.write_report.report.p_data[i]);
                ble_rx_buffer[i] = (char)p_event->ble_event.write_report.report.p_data[i];
            }
            TAL_PR_DEBUG("Recieved >>>>>>>>>> \n %s", ble_rx_buffer);
            break;
        }
        case TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY:
            {
                TAL_PR_DEBUG("EVENT = TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY");
                TAL_BLE_PEER_INFO_T peer_info = p_event->ble_event.data_read.peer;
                TAL_BLE_CONN_PARAMS_T conn_param = p_event->ble_event.conn_param.conn;
                tal_ext_ble_conn_param_update(peer_info, &conn_param);
            }
            break;

        case TAL_BLE_EVT_READ_CHAR:
            {
                TAL_PR_DEBUG("EVENT = TAL_BLE_EVT_READ_CHAR");
                TAL_BLE_PEER_INFO_T peer_info = p_event->ble_event.data_report.peer;
                TAL_BLE_DATA_T data_report = p_event->ble_event.data_report.report;
                char data[] = "hello world";
                data_report.p_data = data;
                data_report.len = strlen(data);
                TAL_PR_DEBUG("data = %s, len = %d", data_report.p_data, data_report.len);
                tal_ext_ble_server_common_read_update_ext(3, &data_report);
            }
            break;

        default: break;
    }
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

    // TUYA_CALL_ERR_LOG(tal_ext_ble_bt_init(TAL_BLE_ROLE_PERIPERAL, ___bal_ble_peripheral_event_callback));

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
    ezlopi_ble_service_init();
    // ezlopi_init();
    // ezlopi_service_broadcast_init();
    // ezlopi_service_web_provisioning_init();
    // ezlopi_nvs_set_boot_count(ezlopi_system_info_get_boot_count() + 1);

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