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
                __esp_bt_gap_callback(ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT, NULL);

                // tal_ext_ble_advertising_data_set(&adv_data, &rsp_data);
                // tal_ext_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);

                
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
                TAL_PR_DEBUG("HANDLE = %d", peer_info.char_handle);
                tal_ext_ble_server_common_read_update_ext(3, &data_report);
            }
            break;

        default: break;
    }
}

void bt_task()
{
    // while(1)
    {
        TAL_PR_DEBUG("bt_task running ......");
        if(NULL == __esp_bt_gatts_callback && NULL == __esp_bt_gap_callback)
        {
            __esp_bt_gatts_callback = esp_ble_gatts_get_callback();
            __esp_bt_gap_callback = esp_ble_gap_get_callback();
        }
        tal_ext_ble_bt_init_v2(TAL_BLE_ROLE_PERIPERAL, ___bal_ble_peripheral_event_callback);
        tal_ext_ble_set_char_param(ptkl_ble_service_char);
        tal_system_sleep(10000);
        TAL_PR_DEBUG("Registering services using APP IDs");

    }
}

esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg)
{
    esp_err_t err;
    uint32_t btdm_cfg_mask = 0;

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

    if (!ptkl_ble_service) {
        ptkl_ble_service = tal_malloc(sizeof(TKL_BLE_SERVICE_PARAMS_T) * TKL_BLE_GATT_SERVICE_MAX_NUM);
    }
    if (!ptkl_ble_service_char) {
        ptkl_ble_service_char = tal_malloc(sizeof(TKL_BLE_CHAR_PARAMS_T) * TKL_BLE_GATT_CHAR_MAX_NUM);
    }

    if ((role & TAL_BLE_ROLE_PERIPERAL) == TAL_BLE_ROLE_PERIPERAL) {
        TKL_BLE_GATTS_PARAMS_T *p_ble_services = &tkl_ble_gatt_service;
        *p_ble_services = (TKL_BLE_GATTS_PARAMS_T){
            .svc_num = 2,
            .p_service = ptkl_ble_service,
        };
        *(ptkl_ble_service) = (TKL_BLE_SERVICE_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .svc_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0x00E3,
                },
            .type = TKL_BLE_UUID_SERVICE_PRIMARY,
            .char_num = 3,
            .p_char = ptkl_ble_service_char,
        };
        *(ptkl_ble_service + 1) = (TKL_BLE_SERVICE_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .svc_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0x00E4,
                },
            .type = TKL_BLE_UUID_SERVICE_PRIMARY,
            .char_num = 1,
            .p_char = (ptkl_ble_service_char + 3),
        };

        /**< Add Write Characteristic */
        *(ptkl_ble_service_char + TAL_COMMON_WRITE_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0xE301,
                },
            .property = TKL_BLE_GATT_CHAR_PROP_WRITE,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len = 244,
        };

        *(ptkl_ble_service_char + TAL_COMMON_WRITE_CHAR_INDEX + 1) = (TKL_BLE_CHAR_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0xE302,
                },
            .property = TKL_BLE_GATT_CHAR_PROP_WRITE,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len = 244,
        };

        *(ptkl_ble_service_char + TAL_COMMON_WRITE_CHAR_INDEX + 2) = (TKL_BLE_CHAR_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0xE303,
                },
            .property = TKL_BLE_GATT_CHAR_PROP_WRITE,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len = 244,
        };

        *(ptkl_ble_service_char + TAL_COMMON_WRITE_CHAR_INDEX + 3) = (TKL_BLE_CHAR_PARAMS_T){
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid =
                {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = 0xE401,
                },
            .property = TKL_BLE_GATT_CHAR_PROP_READ,
            .permission = TKL_BLE_GATT_PERM_READ,
            .value_len = 244,
        };

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