/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"

#include "tal_thread.h"
#include "tal_system.h"
#include "tal_log.h"
#include "tal_ext_bluetooth.h"

#include "bt_event_stack.h"
#include "bt_event_type.h"

#include "bk_bt_hal.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt.h"
#include "esp_err.h"

#if CONFIG_BT_ENABLED

OPERATE_RET tal_ext_ble_bt_init_v2(TAL_BLE_ROLE_E role, const TAL_BLE_EVT_FUNC_CB ble_event);

static esp_gatts_cb_t       __esp_bt_gatts_callback = NULL;
static esp_gap_ble_cb_t     __esp_bt_gap_callback = NULL;

static TKL_BLE_GATTS_PARAMS_T tkl_ble_gatt_service = {0};
static TKL_BLE_SERVICE_PARAMS_T *ptkl_ble_service = NULL;
static TKL_BLE_CHAR_PARAMS_T *ptkl_ble_service_char = NULL;

/* ble callback information */
STATIC TAL_BLE_PEER_INFO_T sg_ble_peripheral_info;
STATIC char ble_rx_buffer[2 * 1024] = {0};

STATIC VOID_T ___bal_ble_peripheral_event_callback(TAL_BLE_EVT_PARAMS_T *p_event)
{
    TAL_PR_DEBUG("----------ble_peripheral event callback-------");
    TAL_PR_DEBUG("ble_peripheral event is : %d", p_event->type);
    switch(p_event->type) {
        case TAL_BLE_STACK_INIT: {
            TAL_PR_DEBUG("init Ble/Bt stack and start advertising.");
            if(p_event->ble_event.init == 0) {
                tal_system_sleep(1000);
                __esp_bt_gap_callback(ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT, NULL);              
            }
            break;
        }
        case TAL_BLE_EVT_PERIPHERAL_CONNECT: {
            TAL_PR_DEBUG("ble_peripheral starts to connect...");
            //FIXME: Commented out this as it is making no difference in functionality. should be fixed in future
            // if(p_event->ble_event.connect.result == 0) {
            //     TAL_BLE_DATA_T read_data;
            //     UCHAR_T read_buffer[512];

            //     memcpy(&sg_ble_peripheral_info, &p_event->ble_event.connect.peer, sizeof(TAL_BLE_PEER_INFO_T));
            //     memcpy(read_buffer, adv_data_const, sizeof(adv_data_const));
            //     memcpy(&read_buffer[sizeof(adv_data_const)], scan_rsp_data_const, sizeof(scan_rsp_data_const));

            //     read_data.p_data = read_buffer;
            //     read_data.len = sizeof(adv_data_const) + sizeof(scan_rsp_data_const);

            //     // Verify Read Char
            //     tal_ext_ble_server_common_read_update(&read_data);
            // } else {
            //     memset(&sg_ble_peripheral_info, 0, sizeof(TAL_BLE_PEER_INFO_T));
            // }
            break;
        }
        case TAL_BLE_EVT_DISCONNECT: {
            TAL_PR_DEBUG("ble_peripheral disconnect.");
            __esp_bt_gatts_callback(ESP_GATTS_DISCONNECT_EVT, 0, NULL); 
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

            esp_ble_gatts_cb_param_t mtu_param;
            mtu_param.mtu.mtu = p_event->ble_event.exchange_mtu.mtu;
            __esp_bt_gatts_callback(ESP_GATTS_MTU_EVT, 0, &mtu_param); 
            break;
        }
        case TAL_BLE_EVT_WRITE_REQ: {
            INT_T i = 0;
            TAL_PR_DEBUG("Get Device-Write Char Request");
            // for ( i = 0; i < p_event->ble_event.write_report.report.len; i++) {
                // TAL_PR_DEBUG("devicr send  data[%d]: %d", i, p_event->ble_event.write_report.report.p_data[i]);
                // ble_rx_buffer[i] = (char)p_event->ble_event.write_report.report.p_data[i];
            // }
            TAL_PR_DEBUG("Recieved >>>>>>>>>> \n %s", ble_rx_buffer);
            esp_ble_gatts_cb_param_t write_param = {0};
            write_param.write.value = p_event->ble_event.write_report.report.p_data; 
            write_param.write.len = p_event->ble_event.write_report.report.len;
            write_param.write.need_rsp = 0;
            // write_param.write.handle = p_event->ble_event.write_report.peer.char_handle[0];
            {
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[0]);
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[1]);
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[2]);
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[3]);
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[4]);
                TAL_PR_DEBUG("HANDLE = %d", p_event->ble_event.write_report.peer.char_handle[5]);
            }
            switch(p_event->ble_event.write_report.peer.char_handle[0])
            {
                case 7:
                    write_param.write.handle = 0;
                    __esp_bt_gatts_callback(ESP_GATTS_WRITE_EVT, 0, &write_param);
                break;
                
                case 3:
                    write_param.write.handle = 1;
                    __esp_bt_gatts_callback(ESP_GATTS_WRITE_EVT, 0, &write_param);
                break;  
                
                case 5:
                    write_param.write.handle = 2;
                    __esp_bt_gatts_callback(ESP_GATTS_WRITE_EVT, 0, &write_param);
                break;
            }

            // __esp_bt_gatts_callback(ESP_GATTS_WRITE_EVT, p_event->ble_event.write_report.peer.char_handle[0], &write_param);

            break;
        }
        case TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY:
            {
                //FIXME: Commented out this as it is making no difference in functionality. should be fixed in future
                // TAL_PR_DEBUG("EVENT = TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY");
                // TAL_BLE_PEER_INFO_T peer_info = p_event->ble_event.data_read.peer;
                // TAL_BLE_CONN_PARAMS_T conn_param = p_event->ble_event.conn_param.conn;
                // tal_ext_ble_conn_param_update(peer_info, &conn_param);
            }
            break;

        case TAL_BLE_EVT_READ_CHAR:
            {
                TAL_PR_DEBUG("EVENT = TAL_BLE_EVT_READ_CHAR");
                TAL_BLE_PEER_INFO_T peer_info = p_event->ble_event.data_report.peer;
                TAL_BLE_DATA_T data_report = p_event->ble_event.data_report.report;
                esp_ble_gatts_cb_param_t read_param = {0};
                read_param.read.handle = 0;
                read_param.read.offset = 0;
                __esp_bt_gatts_callback(ESP_GATTS_READ_EVT, 1, &read_param);

            }
            break;

        default: break;
    }
}

Stack* evt_stack;

void bt_task()
{
    // while(1)
    if (evt_stack == NULL) {
        return 1; // Exit if the stack could not be created
    }

    {
        TAL_PR_DEBUG("bt_task running ......");
        tal_system_sleep(3000);
        if(NULL == __esp_bt_gatts_callback && NULL == __esp_bt_gap_callback)
        {
            TAL_PR_DEBUG("Assigning esp_ gatt gap callbacks");
            __esp_bt_gatts_callback = esp_ble_gatts_get_callback();
            __esp_bt_gap_callback = esp_ble_gap_get_callback();
        }
        while(!isStackEmpty(evt_stack))
        {
            esp_bt_event_t __item = {0};
            pop(evt_stack, &__item);
            int item_index = 0;
            while(__item.num_handles > 1)
            {
                __esp_bt_gatts_callback(ESP_GATTS_START_EVT, __item.gatt_if, NULL);

                esp_ble_gatts_cb_param_t add_char = {0}; // Initialize the struct to zero
                add_char.add_char.attr_handle = item_index;       // Set the desired field

                __esp_bt_gatts_callback(ESP_GATTS_ADD_CHAR_EVT, __item.gatt_if, &add_char);

                TAL_PR_DEBUG("calline __esp_bt_gatts_callback");
                __item.num_handles -= 2;
                item_index++;
            }
            TAL_PR_DEBUG("\n");

        }
        tal_ext_ble_bt_init_v2(TAL_BLE_ROLE_PERIPERAL, ___bal_ble_peripheral_event_callback);
        TAL_PR_DEBUG("Registering services using APP IDs");

    }
}

void* esp_bt_get_event_stack_ptr()
{
    return (void*)evt_stack;
}

esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg)
{
    esp_err_t err;
    uint32_t btdm_cfg_mask = 0;
    evt_stack = createStack(10, sizeof(esp_bt_event_t));

    THREAD_HANDLE ezlopi_service_timer_task_handle = NULL;
    THREAD_CFG_T thread_cfg = {cfg->controller_task_stack_size, cfg->controller_task_prio,"bt_task"};
    tal_thread_create_and_start(&ezlopi_service_timer_task_handle, NULL, NULL, bt_task, NULL, &thread_cfg);

    return err;
}

/**
 * @brief   Function for initializing the bluetooth
 * @param   [in] role:      set the ble/bt role
 *          [in] p_event:   Event handler provided by the user.
 * @return  SUCCESS         Initialized successfully.
 *          ERROR
 * */
OPERATE_RET tal_ext_ble_bt_init_v2(TAL_BLE_ROLE_E role, const TAL_BLE_EVT_FUNC_CB ble_event)
{
    uint8_t ble_stack_role = 0; // TKL_BLE_ROLE_SERVER;

    if (ble_event != NULL) {
        /**<  Get the TAL Event Callback. */
        tal_ext_ble_set_ble_event_callback(ble_event);

        /**< Register GAP And GATT Callback */
        tkl_ble_gap_callback_register(tal_ext_ble_get_kernal_gap_event_callback());
        tkl_ble_gatt_callback_register(tal_ext_ble_get_kernal_gatt_event_callback());
    }

    /**< Init Bluetooth Stack Role For Ble. */
    if ((role & TAL_BLE_ROLE_PERIPERAL) == TAL_BLE_ROLE_PERIPERAL ||
        (role & TAL_BLE_ROLE_BEACON) == TAL_BLE_ROLE_BEACON) {
        ble_stack_role |= TKL_BLE_ROLE_SERVER;
    }
    if ((role & TAL_BLE_ROLE_CENTRAL) == TAL_BLE_ROLE_CENTRAL) {
        ble_stack_role |= TKL_BLE_ROLE_CLIENT;
    }

    if ((role & TAL_BLE_ROLE_PERIPERAL) == TAL_BLE_ROLE_PERIPERAL) {
        TKL_BLE_GATTS_PARAMS_T *p_ble_services = bk_bt_hal_start_services();
        tal_ext_ble_set_service_ptr(p_ble_services->p_service);
        tal_ext_ble_set_char_ptr(p_ble_services->p_service->p_char);
        if (tkl_ble_gatts_service_add(p_ble_services) != 0) {
            return OPRT_OS_ADAPTER_BLE_INIT_FAILED;
        }
    }

    if (tkl_ble_stack_init(ble_stack_role) != OPRT_OK) {
        return OPRT_OS_ADAPTER_BLE_INIT_FAILED;
    }
    
    return OPRT_OK;
}

#endif /*  CONFIG_BT_ENABLED */