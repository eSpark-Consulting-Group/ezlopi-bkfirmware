/**
 * @file bal_bluetooth.c
 * @brief This is bal_bluetooth file
 * @version 1.0
 * @date 2023-09-26
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include <string.h>                     // [20220215] Use Standard C Library.
#include "bal_bluetooth_def.h"
#include "tkl_bluetooth_def.h"
#include "tkl_bluetooth.h"
#include "tal_log.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define BAL_COMMON_SERVICE_INDEX        (0)

#define BAL_COMMON_SERVICE_MAX_NUM      (1)
#define BAL_COMMON_CHAR_MAX_NUM         (3)

#ifndef BAL_BLE_SERVICE_VERSION
#define BAL_BLE_SERVICE_VERSION         (2)     // Default Service is version 2: Follow the latest service.
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
/**< Use TKL Definitions */
STATIC TKL_BLE_GATTS_PARAMS_T           tkl_ble_gatt_service = {0};
STATIC TKL_BLE_SERVICE_PARAMS_T         tkl_ble_common_service[4] = {0};    // [20220531] Remove TKL Macro Use
STATIC TKL_BLE_CHAR_PARAMS_T            tkl_ble_common_char[6] = {0};       // [20220531] Remove TKL Macro Use
STATIC USHORT_T                         tkl_ble_common_connect_handle = TKL_BLE_GATT_INVALID_HANDLE;

/**< Use BAL Definitions, BAL Only Support One GATT Link at one time, follow Bluetooth Spec. */
STATIC UCHAR_T                          bal_ble_disc_svc_busy = 0;
STATIC BAL_BLE_PEER_INFO_T              bal_ble_peer = {0};
STATIC BAL_BLE_EVT_FUNC_CB              bal_ble_event_callback;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




STATIC USHORT_T bal_ble_uuid16_convert(TKL_BLE_UUID_T *p_uuid)
{
    USHORT_T uuid16 = 0xFFFF;

    if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_16) {
        return p_uuid->uuid.uuid16;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_32) {
        /**< Not Support */
        return 0xFFFF;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_128) {
        uuid16 = p_uuid->uuid.uuid128[13] << 8 |  p_uuid->uuid.uuid128[12];
        return uuid16;
    }

    return 0xFFFF;
}

STATIC VOID_T tkl_ble_kernel_gap_event_callback(TKL_BLE_GAP_PARAMS_EVT_T *p_event)
{
    BAL_BLE_EVT_PARAMS_T bal_event;

    if (p_event == NULL) return;

    memset(&bal_event, 0, sizeof(BAL_BLE_EVT_PARAMS_T));

    switch (p_event->type) {
        case TKL_BLE_EVT_STACK_INIT: {
            bal_event.type              = BAL_BLE_STACK_INIT;
            bal_event.ble_event.init    = p_event->result;
        } break;

        /**< Deinit Event Callback From Ble Adapter */
        case TKL_BLE_EVT_STACK_DEINIT: {
            bal_event.type              = BAL_BLE_STACK_DEINIT;
            bal_event.ble_event.init    = p_event->result;
        } break;

        /**< Reset Event Callback From Ble Adapter */
        case TKL_BLE_EVT_STACK_RESET: {
            bal_event.type              = BAL_BLE_STACK_RESET;
            bal_event.ble_event.init    = p_event->result;
        } break;

        case TKL_BLE_GAP_EVT_CONNECT: {
            bal_event.ble_event.connect.result  = p_event->result;
            if (p_event->gap_event.connect.role  == TKL_BLE_ROLE_SERVER) {
                bal_event.type = BAL_BLE_EVT_PERIPHERAL_CONNECT;

                tkl_ble_common_connect_handle                   = p_event->conn_handle;
                bal_event.ble_event.connect.peer.conn_handle    = p_event->conn_handle;

                bal_event.ble_event.connect.peer.char_handle[BAL_COMMON_WRITE_CHAR_INDEX]   = tkl_ble_common_char[BAL_COMMON_WRITE_CHAR_INDEX].handle;
                bal_event.ble_event.connect.peer.char_handle[BAL_COMMON_NOTIFY_CHAR_INDEX]  = tkl_ble_common_char[BAL_COMMON_NOTIFY_CHAR_INDEX].handle;
                bal_event.ble_event.connect.peer.char_handle[BAL_COMMON_READ_CHAR_INDEX]    = tkl_ble_common_char[BAL_COMMON_READ_CHAR_INDEX].handle;
            } else {
                if (p_event->result != OPRT_OK) {
                    bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
                    bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_GATT_CONN_FAILED;
                    memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
                } else {
                    bal_ble_peer.conn_handle = p_event->conn_handle;
                    if (0) {  // Try To Discovery All Service, And Find the correct service
                        bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
                        bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;
                        memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
                    } else {
                        /* We will still be in service finding procedure. Should not report any states into BAL Application */
                        bal_ble_disc_svc_busy = 1;
                        return;
                    }
                }
            }
        } break;

        case TKL_BLE_GAP_EVT_DISCONNECT: {
            if (p_event->gap_event.disconnect.role == TKL_BLE_ROLE_SERVER && p_event->conn_handle == tkl_ble_common_connect_handle) {
                tkl_ble_common_connect_handle = TKL_BLE_GATT_INVALID_HANDLE;
            } else if (p_event->gap_event.disconnect.role == TKL_BLE_ROLE_CLIENT && p_event->conn_handle == bal_ble_peer.conn_handle
                    && bal_ble_disc_svc_busy) {
                /* [20220526][TimCheng]
                    当已存在GATT连接，发现服务过程中突然断开时，我们可能无法收到失败的服务发现，但是会收到断开事件，此时我们需要上报该情况到业务端。
                    触发该条件仅存在于服务发现过程中且先收到断开事件，当已上报服务发现状态到业务端时，我们无需重复上报服务发现失败。
                */
                bal_ble_disc_svc_busy = 0;
                bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
                bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;
                memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));

                if (bal_ble_event_callback) {
                    bal_ble_event_callback(&bal_event);
                }
                return;
            }

            bal_event.type                                      = BAL_BLE_EVT_DISCONNECT;
            bal_event.ble_event.disconnect.peer.conn_handle     = p_event->conn_handle;
            bal_event.ble_event.disconnect.reason               = p_event->gap_event.disconnect.reason;
        } break;

        case TKL_BLE_GAP_EVT_ADV_REPORT: {
            bal_event.type                                      = BAL_BLE_EVT_ADV_REPORT;
            bal_event.ble_event.adv_report.rssi                 = p_event->gap_event.adv_report.rssi;
            bal_event.ble_event.adv_report.data_len             = (UCHAR_T)p_event->gap_event.adv_report.data.length;
            bal_event.ble_event.adv_report.p_data               = p_event->gap_event.adv_report.data.p_data;
            memcpy(bal_event.ble_event.adv_report.peer_addr.addr, p_event->gap_event.adv_report.peer_addr.addr, 6);

            if (p_event->gap_event.adv_report.adv_type == TKL_BLE_ADV_DATA) {
                bal_event.ble_event.adv_report.adv_type = BAL_BLE_ADV_DATA;
            } else if (p_event->gap_event.adv_report.adv_type == TKL_BLE_RSP_DATA) {
                bal_event.ble_event.adv_report.adv_type = BAL_BLE_RSP_DATA;
            } else if (p_event->gap_event.adv_report.adv_type == TKL_BLE_ADV_RSP_DATA) {
                bal_event.ble_event.adv_report.adv_type = BAL_BLE_ADV_RSP_DATA;
            } else if (p_event->gap_event.adv_report.adv_type == TKL_BLE_NONCONN_ADV_DATA) {
                bal_event.ble_event.adv_report.adv_type = BAL_BLE_NONCONN_ADV_DATA;
            } else if (p_event->gap_event.adv_report.adv_type == TKL_BLE_EXTENDED_ADV_DATA) {
                bal_event.ble_event.adv_report.adv_type = BAL_BLE_EXTENDED_ADV_DATA;
            }

            if (p_event->gap_event.adv_report.peer_addr.type == TKL_BLE_GAP_ADDR_TYPE_PUBLIC) {
                bal_event.ble_event.adv_report.peer_addr.type   = BAL_BLE_ADDR_TYPE_PUBLIC;
            } else if (p_event->gap_event.adv_report.peer_addr.type == TKL_BLE_GAP_ADDR_TYPE_RANDOM) {
                bal_event.ble_event.adv_report.peer_addr.type   = BAL_BLE_ADDR_TYPE_RANDOM;
            } else {
                /**< Currently, we not support other types. And Tuya Bluetooth Device will not use other types. */
                return;
            }
        } break;

        case TKL_BLE_GAP_EVT_CONN_PARAM_REQ: {
            bal_event.type                                      = BAL_BLE_EVT_CONN_PARAM_REQ;
            bal_event.ble_event.conn_param.conn_handle          = p_event->conn_handle;
        } break;

        case TKL_BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            bal_event.type                                      = BAL_BLE_EVT_CONN_PARAM_UPDATE;
            bal_event.ble_event.conn_param.conn_handle          = p_event->conn_handle;

            bal_event.ble_event.conn_param.conn.conn_sup_timeout    = p_event->gap_event.conn_param.conn_sup_timeout;
            bal_event.ble_event.conn_param.conn.max_conn_interval   = p_event->gap_event.conn_param.conn_interval_max;
            bal_event.ble_event.conn_param.conn.min_conn_interval   = p_event->gap_event.conn_param.conn_interval_min;
            bal_event.ble_event.conn_param.conn.latency             = p_event->gap_event.conn_param.conn_latency;
        } break;

        case TKL_BLE_GAP_EVT_CONN_RSSI: {
            bal_event.type                                      = BAL_BLE_EVT_CONN_RSSI;
            bal_event.ble_event.link_rssi.conn_handle           = p_event->conn_handle;

            bal_event.ble_event.link_rssi.rssi                  = p_event->gap_event.link_rssi;
        } break;

        default: {

        } return;
    }

    if (bal_ble_event_callback) {
        bal_ble_event_callback(&bal_event);
    }
}

STATIC VOID_T tkl_ble_kernel_gatt_event_callback(TKL_BLE_GATT_PARAMS_EVT_T *p_event)
{
    BAL_BLE_EVT_PARAMS_T bal_event;

    if (p_event == NULL) return;

    memset(&bal_event, 0, sizeof(BAL_BLE_EVT_PARAMS_T));

    switch (p_event->type) {
        case TKL_BLE_GATT_EVT_MTU_REQUEST: {
            bal_event.type = BAL_BLE_EVT_MTU_REQUEST;
            bal_event.ble_event.exchange_mtu.conn_handle        = p_event->conn_handle;
            bal_event.ble_event.exchange_mtu.mtu                = p_event->gatt_event.exchange_mtu;
        } break;

        case TKL_BLE_GATT_EVT_MTU_RSP: {
            bal_event.type = BAL_BLE_EVT_MTU_RSP;
            bal_event.ble_event.exchange_mtu.conn_handle        = p_event->conn_handle;
            bal_event.ble_event.exchange_mtu.mtu                = p_event->gatt_event.exchange_mtu;
        } break;

        // case TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY: {
        //     TAL_PR_DEBUG("EVENT : TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY");
        //     TKL_BLE_GATT_SVC_DISC_TYPE_T *p_svc_disc = &p_event->gatt_event.svc_disc;
        //     INT_T svc_result = OPRT_INVALID_PARM;

        //     bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
        //     if (p_event->result == OPRT_OK) {

        //         /**< Find the All Characteristic. */
        //         TAL_PR_DEBUG("Find the All Characteristic");
        //         UCHAR_T i = 0;
        //         for (i = 0; i < p_svc_disc->svc_num; i ++) {
        //             TAL_PR_DEBUG("Serive %d", i);
        //             if (bal_ble_uuid16_convert(&p_svc_disc->services[i].uuid) == BAL_BLE_CMD_SERVICE_UUID_V1
        //                     || bal_ble_uuid16_convert(&p_svc_disc->services[i].uuid) == BAL_BLE_CMD_SERVICE_UUID_V2) {
        //                 // svc_result = tkl_ble_gattc_all_char_discovery(p_event->conn_handle, p_svc_disc->services[i].start_handle, p_svc_disc->services[i].end_handle);
        //                 // if (svc_result == 0) {
        //                 //     return;
        //                 // }
        //             }
        //         }
        //         memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;
        //     } else {
        //         /**< Return Error, Cannot Discovery Service */
        //         memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         bal_event.ble_event.connect.result = p_event->result;
        //     }
        // } break;

        // case TKL_BLE_GATT_EVT_CHAR_DISCOVERY: {
        //     TAL_PR_DEBUG("EVENT : TKL_BLE_GATT_EVT_CHAR_DISCOVERY");
        //     TKL_BLE_GATT_CHAR_DISC_TYPE_T *p_char_disc = &p_event->gatt_event.char_disc;
        //     INT_T char_result = OPRT_INVALID_PARM;

        //     bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
        //     if (p_event->result == OPRT_OK) {
        //         UCHAR_T i = 0;
        //         for (i = 0; i < p_char_disc->char_num; i ++) {
        //             if (bal_ble_uuid16_convert(&p_char_disc->characteristics[i].uuid) == BAL_BLE_CMD_WRITE_CHAR_UUID_V1
        //                 || bal_ble_uuid16_convert(&p_char_disc->characteristics[i].uuid) == BAL_BLE_CMD_WRITE_CHAR_UUID_V2) {

        //                 bal_ble_peer.char_handle[BAL_COMMON_WRITE_CHAR_INDEX] = p_char_disc->characteristics[i].handle;
        //             } else if (bal_ble_uuid16_convert(&p_char_disc->characteristics[i].uuid) == BAL_BLE_CMD_NOTIFY_CHAR_UUID_V1
        //                 || bal_ble_uuid16_convert(&p_char_disc->characteristics[i].uuid) == BAL_BLE_CMD_NOTIFY_CHAR_UUID_V2) {

        //                 bal_ble_peer.char_handle[BAL_COMMON_NOTIFY_CHAR_INDEX] = p_char_disc->characteristics[i].handle;
        //             } else if (bal_ble_uuid16_convert(&p_char_disc->characteristics[i].uuid) == BAL_BLE_CMD_READ_CHAR_UUID_V2) {

        //                 bal_ble_peer.char_handle[BAL_COMMON_READ_CHAR_INDEX] = p_char_disc->characteristics[i].handle;
        //             }
        //         }

        //         /**< Find Characteristic Descriptor */
        //         if (bal_ble_peer.char_handle[BAL_COMMON_NOTIFY_CHAR_INDEX] != TKL_BLE_GATT_INVALID_HANDLE) {
        //             // char_result = tkl_ble_gattc_char_desc_discovery(p_event->conn_handle, bal_ble_peer.char_handle[BAL_COMMON_NOTIFY_CHAR_INDEX], 0xFFFF);
        //             // if (char_result == OPRT_OK) {
        //             //     return;
        //             // }
        //             return;
        //         }

        //         memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_DESC_DISC_FAILED;
        //     } else {
        //         /**< Return Error, Cannot Discovery Service */
        //         memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         bal_event.ble_event.connect.result = p_event->result;
        //     }
        // } break;

        // case TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY: {
        //     TAL_PR_DEBUG("EVENT : TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY");
        //     TKL_BLE_GATT_DESC_DISC_TYPE_T *p_desc_disc = &p_event->gatt_event.desc_disc;
        //     UCHAR_T cccd_enable[2] = {0x01, 0x00};
        //     INT_T desc_result;

        //     bal_event.type = BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY;
        //     if (p_event->result == OPRT_OK) {
        //         desc_result = tkl_ble_gattc_write_without_rsp(p_event->conn_handle, p_desc_disc->cccd_handle, cccd_enable, sizeof(cccd_enable));
        //         if (0) {
        //             bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_DESC_DISC_FAILED;
        //         } else if (p_event->conn_handle != bal_ble_peer.conn_handle) {
        //             bal_event.ble_event.connect.result = OPRT_OS_ADAPTER_BLE_HANDLE_ERROR;
        //             memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         } else {
        //             bal_event.ble_event.connect.result = OPRT_OK;
        //             memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //         }
        //     } else {
        //         /**< Return Error, Cannot Discovery characteristic descriptor */
        //         bal_event.ble_event.connect.result = p_event->result;
        //         memcpy(&bal_event.ble_event.connect.peer , &bal_ble_peer, sizeof(BAL_BLE_PEER_INFO_T));
        //     }
        // } break;

        case TKL_BLE_GATT_EVT_NOTIFY_TX: {
            bal_event.type = BAL_BLE_EVT_NOTIFY_TX;
            bal_event.ble_event.notify_result.conn_handle           = p_event->conn_handle;

            bal_event.ble_event.notify_result.char_handle           = p_event->gatt_event.notify_result.char_handle;
            bal_event.ble_event.notify_result.result                = p_event->gatt_event.notify_result.result;
        } break;

        case TKL_BLE_GATT_EVT_WRITE_REQ: {
            bal_event.type = BAL_BLE_EVT_WRITE_REQ;
            bal_event.ble_event.write_report.peer.conn_handle       = p_event->conn_handle;
            bal_event.ble_event.write_report.peer.char_handle[0]    = p_event->gatt_event.write_report.char_handle;
            bal_event.ble_event.write_report.report.len             = p_event->gatt_event.write_report.report.length;
            bal_event.ble_event.write_report.report.p_data          = p_event->gatt_event.write_report.report.p_data;
        } break;

        case TKL_BLE_GATT_EVT_NOTIFY_INDICATE_RX: {
            bal_event.type = BAL_BLE_EVT_NOTIFY_RX;
            bal_event.ble_event.data_report.peer.conn_handle        = p_event->conn_handle;
            bal_event.ble_event.data_report.peer.char_handle[0]     = p_event->gatt_event.data_report.char_handle;
            bal_event.ble_event.data_report.report.len              = p_event->gatt_event.data_report.report.length;
            bal_event.ble_event.data_report.report.p_data           = p_event->gatt_event.data_report.report.p_data;
        } break;

        case TKL_BLE_GATT_EVT_READ_RX: {
            bal_event.type = BAL_BLE_EVT_READ_RX;
            bal_event.ble_event.data_read.peer.conn_handle          = p_event->conn_handle;
            bal_event.ble_event.data_read.peer.char_handle[0]       = p_event->gatt_event.data_read.char_handle;
            bal_event.ble_event.data_read.report.len                = p_event->gatt_event.data_read.report.length;
            bal_event.ble_event.data_read.report.p_data             = p_event->gatt_event.data_read.report.p_data;
        } break;

        case TKL_BLE_GATT_EVT_SUBSCRIBE: {
            if (p_event->result == OPRT_OK) {
                bal_event.type = BAL_BLE_EVT_SUBSCRIBE;
                bal_event.ble_event.subscribe.conn_handle = p_event->conn_handle;
                bal_event.ble_event.subscribe.char_handle = p_event->gatt_event.subscribe.char_handle;
                bal_event.ble_event.subscribe.reason = p_event->gatt_event.subscribe.reason;
                bal_event.ble_event.subscribe.prev_notify = p_event->gatt_event.subscribe.prev_notify;
                bal_event.ble_event.subscribe.cur_notify = p_event->gatt_event.subscribe.cur_notify;
                bal_event.ble_event.subscribe.prev_indicate = p_event->gatt_event.subscribe.prev_indicate;
                bal_event.ble_event.subscribe.cur_indicate = p_event->gatt_event.subscribe.cur_indicate;
            }
        }break;

        case TKL_BLE_GATT_EVT_READ_CHAR_VALUE: {
            bal_event.type = BAL_BLE_EVT_READ_CHAR;
            bal_event.ble_event.char_read.conn_handle = p_event->conn_handle;
            bal_event.ble_event.char_read.char_handle = p_event->gatt_event.char_read.char_handle;
            bal_event.ble_event.char_read.offset = p_event->gatt_event.char_read.offset;
        }break;

        default: {
            // Not Support
        } return;
    }

    // 【20220526】【TimCheng】：仅对Ble Central有效。
    // 新增标志说明当前是否处于服务发现状态繁忙，用于表明GATT已连接但是未正确发现服务但是设备已断开时，我们将主动上报该状态至业务端。
    if (bal_event.type == BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY) bal_ble_disc_svc_busy = 0;

    if (bal_ble_event_callback) {
        bal_ble_event_callback(&bal_event);
    }
}

OPERATE_RET bal_ble_bt_init(BAL_BLE_ROLE_E role, CONST BAL_BLE_EVT_FUNC_CB ble_event)
{
    UCHAR_T ble_stack_role = 0x00; //TKL_BLE_ROLE_SERVER;    // Default Value Should be zero if using in "Single" mode

    if (ble_event != NULL) {
        /**<  Get the BAL Event Callback. */
        bal_ble_event_callback = ble_event;

        /**< Register GAP And GATT Callback */
        tkl_ble_gap_callback_register(tkl_ble_kernel_gap_event_callback);
        tkl_ble_gatt_callback_register(tkl_ble_kernel_gatt_event_callback);
    }

    /**< Init Bluetooth Stack Role For Ble. */
    if ((role&BAL_BLE_ROLE_PERIPERAL) == BAL_BLE_ROLE_PERIPERAL || (role&BAL_BLE_ROLE_BEACON) == BAL_BLE_ROLE_BEACON) {
        ble_stack_role |= TKL_BLE_ROLE_SERVER;
    }
    if ((role&BAL_BLE_ROLE_CENTRAL) == BAL_BLE_ROLE_CENTRAL) {
        ble_stack_role |= TKL_BLE_ROLE_CLIENT;
    }

    if (tkl_ble_stack_init(ble_stack_role) != OPRT_OK) {
        return OPRT_OS_ADAPTER_BLE_INIT_FAILED;
    }

    if ((role&BAL_BLE_ROLE_PERIPERAL) == BAL_BLE_ROLE_PERIPERAL) {
        TKL_BLE_GATTS_PARAMS_T *p_ble_services = &tkl_ble_gatt_service;
        *p_ble_services = (TKL_BLE_GATTS_PARAMS_T) {
            .svc_num    = BAL_COMMON_SERVICE_MAX_NUM,
            .p_service  = tkl_ble_common_service,
        };

#if (defined(BAL_BLE_SERVICE_VERSION)  && (BAL_BLE_SERVICE_VERSION == 2))
        /**< Add Tuya Common Service */
        TKL_BLE_SERVICE_PARAMS_T *p_ble_common_service = tkl_ble_common_service;
        *(p_ble_common_service + BAL_COMMON_SERVICE_INDEX) = (TKL_BLE_SERVICE_PARAMS_T) {
            .handle     = TKL_BLE_GATT_INVALID_HANDLE,
            .svc_uuid   = {
                .uuid_type   = TKL_BLE_UUID_TYPE_16,
                .uuid.uuid16 = BAL_BLE_CMD_SERVICE_UUID_V2,
            },
            .type       = TKL_BLE_UUID_SERVICE_PRIMARY,
            .char_num   = BAL_COMMON_CHAR_MAX_NUM,
            .p_char     = tkl_ble_common_char,
        };

        /**< Add Write Characteristic */
        TKL_BLE_CHAR_PARAMS_T *p_ble_common_char = tkl_ble_common_char;

        *(p_ble_common_char + BAL_COMMON_WRITE_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T) {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid  = {
                .uuid_type   = TKL_BLE_UUID_TYPE_128,
                .uuid.uuid128 = BAL_BLE_CMD_WRITE_CHAR_UUID128_V2,
            },
            .property   = TKL_BLE_GATT_CHAR_PROP_WRITE | TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len  = 244,
        };

        /**< Add Notify Characteristic */
        *(p_ble_common_char + BAL_COMMON_NOTIFY_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T) {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid  = {
                .uuid_type   = TKL_BLE_UUID_TYPE_128,
                .uuid.uuid128 = BAL_BLE_CMD_NOTIFY_CHAR_UUID128_V2,
            },
            .property   = TKL_BLE_GATT_CHAR_PROP_NOTIFY,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len  = 244,
        };

        /**< Add Read Characteristic */
        *(p_ble_common_char + BAL_COMMON_READ_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T) {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid  = {
                .uuid_type   = TKL_BLE_UUID_TYPE_128,
                .uuid.uuid128 = BAL_BLE_CMD_READ_CHAR_UUID128_V2,
            },
            .property   = TKL_BLE_GATT_CHAR_PROP_READ,
            .permission = TKL_BLE_GATT_PERM_READ,
            .value_len  = 244,
        };
#else
        /**< Add Tuya Common Service */
        TKL_BLE_SERVICE_PARAMS_T *p_ble_common_service = tkl_ble_common_service;
        *(p_ble_common_service + BAL_COMMON_SERVICE_INDEX) = (TKL_BLE_SERVICE_PARAMS_T) {
            .handle     = TKL_BLE_GATT_INVALID_HANDLE,
            .svc_uuid   = {
                .uuid_type   = TKL_BLE_UUID_TYPE_16,
                .uuid.uuid16 = BAL_BLE_CMD_SERVICE_UUID_V1,
            },
            .type       = TKL_BLE_UUID_SERVICE_PRIMARY,
            .char_num   = 2,
            .p_char     = tkl_ble_common_char,
        };

        /**< Add Write Characteristic */
        TKL_BLE_CHAR_PARAMS_T *p_ble_common_char = tkl_ble_common_char;

        *(p_ble_common_char + BAL_COMMON_WRITE_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T) {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid  = {
                .uuid_type   = TKL_BLE_UUID_TYPE_16,
                .uuid.uuid16 = BAL_BLE_CMD_WRITE_CHAR_UUID_V1,
            },
            .property   = TKL_BLE_GATT_CHAR_PROP_WRITE | TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len  = 244,
        };

        /**< Add Notify Characteristic */
        *(p_ble_common_char + BAL_COMMON_NOTIFY_CHAR_INDEX) = (TKL_BLE_CHAR_PARAMS_T) {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,
            .char_uuid  = {
                .uuid_type   = TKL_BLE_UUID_TYPE_16,
                .uuid.uuid16 = BAL_BLE_CMD_NOTIFY_CHAR_UUID_V1,
            },
            .property   = TKL_BLE_GATT_CHAR_PROP_NOTIFY,
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,
            .value_len  = 244,
        };

#endif
        if (tkl_ble_gatts_service_add(p_ble_services) != 0) {
            return OPRT_OS_ADAPTER_BLE_INIT_FAILED;
        }
    }

    return OPRT_OK;
}

OPERATE_RET bal_ble_bt_deinit(BAL_BLE_ROLE_E role)
{
    return tkl_ble_stack_deinit(role);
}

OPERATE_RET bal_ble_address_set(BAL_BLE_ADDR_T CONST *p_addr)
{
    TKL_BLE_GAP_ADDR_T ble_addr = {0};

    if (p_addr == NULL) {
        return OPRT_INVALID_PARM;
    }

    ble_addr.type = p_addr->type;
    memcpy(ble_addr.addr, p_addr->addr, 6);
    return tkl_ble_gap_addr_set(&ble_addr);
}

OPERATE_RET bal_ble_address_get(BAL_BLE_ADDR_T *p_addr)
{
    TKL_BLE_GAP_ADDR_T bal_addr = {0};

    if (p_addr == NULL) {
        return OPRT_INVALID_PARM;
    }

    tkl_ble_gap_address_get(&bal_addr);

    p_addr->type = bal_addr.type;
    memcpy(p_addr->addr, bal_addr.addr, 6);

    return OPRT_OK;
}

OPERATE_RET bal_ble_bt_link_max(USHORT_T *p_maxlink)
{
    if (p_maxlink == NULL) {
        return OPRT_INVALID_PARM;
    }

    tkl_ble_stack_gatt_link(p_maxlink);

    return OPRT_OK;
}

OPERATE_RET bal_ble_advertising_start(BAL_BLE_ADV_PARAMS_T CONST *p_adv_param)
{
    TKL_BLE_GAP_ADV_PARAMS_T bal_adv_params = {0};

    if (p_adv_param == NULL) {
        return OPRT_INVALID_PARM;
    }

    bal_adv_params.adv_type             = p_adv_param->adv_type;
    bal_adv_params.direct_addr.type     = p_adv_param->direct_addr.type;
    bal_adv_params.adv_interval_min     = p_adv_param->adv_interval_min;
    bal_adv_params.adv_interval_max     = p_adv_param->adv_interval_max;
    bal_adv_params.adv_channel_map      = 0x01 | 0x02 | 0x04;

    memcpy(bal_adv_params.direct_addr.addr, p_adv_param->direct_addr.addr, 6);

    return tkl_ble_gap_adv_start(&bal_adv_params);
}

OPERATE_RET bal_ble_advertising_data_set(BAL_BLE_DATA_T *p_adv, BAL_BLE_DATA_T *p_scan_rsp)
{
    TKL_BLE_DATA_T adv_data = {0};
    TKL_BLE_DATA_T scan_rsp_data = {0};

    if (p_adv != NULL) {
        adv_data.length = p_adv->len;
        adv_data.p_data = p_adv->p_data;
    } else {
        adv_data.length = 0;
        adv_data.p_data = NULL;
    }

    if (p_scan_rsp != NULL) {
        scan_rsp_data.length = p_scan_rsp->len;
        scan_rsp_data.p_data = p_scan_rsp->p_data;
    } else {
        scan_rsp_data.length = 0;
        scan_rsp_data.p_data = NULL;
    }

    return tkl_ble_gap_adv_rsp_data_set(&adv_data, &scan_rsp_data);
}

OPERATE_RET bal_ble_advertising_stop(VOID_T)
{
    return tkl_ble_gap_adv_stop();
}

OPERATE_RET bal_ble_advertising_data_update(BAL_BLE_DATA_T *p_adv, BAL_BLE_DATA_T *p_scan_rsp)
{
    TKL_BLE_DATA_T adv_data = {0};
    TKL_BLE_DATA_T scan_rsp_data = {0};

    if (p_adv != NULL) {
        adv_data.length = p_adv->len;
        adv_data.p_data = p_adv->p_data;
    } else {
        adv_data.length = 0;
        adv_data.p_data = NULL;
    }

    if (p_scan_rsp != NULL) {
        scan_rsp_data.length = p_scan_rsp->len;
        scan_rsp_data.p_data = p_scan_rsp->p_data;
    } else {
        scan_rsp_data.length = 0;
        scan_rsp_data.p_data = NULL;
    }

    return tkl_ble_gap_adv_rsp_data_update(&adv_data, &scan_rsp_data);
}

OPERATE_RET bal_ble_scan_start(BAL_BLE_SCAN_PARAMS_T CONST *p_scan_param)
{
    TKL_BLE_GAP_SCAN_PARAMS_T bal_scan_params = {0};

    bal_scan_params.extended            = 0;
    bal_scan_params.active              = ((p_scan_param->type == BAL_BLE_SCAN_TYPE_ACTIVE) ? 1 : 0);
    bal_scan_params.scan_phys           = TKL_BLE_GAP_PHY_1MBPS;
    bal_scan_params.interval            = p_scan_param->scan_interval;
    bal_scan_params.window              = p_scan_param->scan_window;
    bal_scan_params.timeout             = p_scan_param->timeout;
    bal_scan_params.scan_channel_map    = 0x01 | 0x02 | 0x04;

    return tkl_ble_gap_scan_start(&bal_scan_params);
}

OPERATE_RET bal_ble_scan_stop(VOID_T)
{
    return tkl_ble_gap_scan_stop();
}

OPERATE_RET bal_ble_rssi_get(CONST BAL_BLE_PEER_INFO_T peer)
{
    USHORT_T conn_handle = peer.conn_handle;

    return tkl_ble_gap_rssi_get(conn_handle);
}

OPERATE_RET bal_ble_conn_param_update(CONST BAL_BLE_PEER_INFO_T peer, BAL_BLE_CONN_PARAMS_T CONST *p_conn_params)
{
    USHORT_T conn_handle = peer.conn_handle;
    TKL_BLE_GAP_CONN_PARAMS_T param = {0};

    if (p_conn_params == NULL) {
        return OPRT_INVALID_PARM;
    }

    param.conn_interval_min     = p_conn_params->min_conn_interval;
    param.conn_interval_max     = p_conn_params->max_conn_interval;
    param.conn_latency          = p_conn_params->latency;
    param.conn_sup_timeout      = p_conn_params->conn_sup_timeout;
    param.connection_timeout    = p_conn_params->connection_timeout;

    return tkl_ble_gap_conn_param_update(conn_handle, &param);
}

OPERATE_RET bal_ble_connect_and_discovery(CONST BAL_BLE_PEER_INFO_T peer, BAL_BLE_CONN_PARAMS_T CONST *p_conn_params)
{
    TKL_BLE_GAP_ADDR_T bal_conn_addr = {0};
    TKL_BLE_GAP_SCAN_PARAMS_T bal_scan_params = {0};
    TKL_BLE_GAP_CONN_PARAMS_T bal_conn_params = {0};

    if (p_conn_params == NULL) {
        bal_conn_params.conn_interval_max   = 0x12;
        bal_conn_params.conn_interval_min   = 0x12;
        bal_conn_params.conn_latency        = 0;
        bal_conn_params.conn_sup_timeout    = 0x100;
    } else {
        bal_conn_params.conn_interval_max   = p_conn_params->max_conn_interval;
        bal_conn_params.conn_interval_min   = p_conn_params->min_conn_interval;
        bal_conn_params.conn_latency        = p_conn_params->latency;
        bal_conn_params.conn_sup_timeout    = p_conn_params->conn_sup_timeout;
    }

    bal_scan_params.interval            = 0x0010;
    bal_scan_params.window              = 0x0010;

    if (peer.peer_addr.type == BAL_BLE_ADDR_TYPE_RANDOM) {
        bal_conn_addr.type = TKL_BLE_GAP_ADDR_TYPE_RANDOM;
    } else {
        bal_conn_addr.type = TKL_BLE_GAP_ADDR_TYPE_PUBLIC;
    }
    memcpy(bal_conn_addr.addr, peer.peer_addr.addr, 6);

    memset(&bal_ble_peer, 0, sizeof(BAL_BLE_PEER_INFO_T));

    bal_ble_peer.char_handle[BAL_COMMON_WRITE_CHAR_INDEX]   = TKL_BLE_GATT_INVALID_HANDLE;
    bal_ble_peer.char_handle[BAL_COMMON_NOTIFY_CHAR_INDEX]  = TKL_BLE_GATT_INVALID_HANDLE;
    bal_ble_peer.char_handle[BAL_COMMON_READ_CHAR_INDEX]    = TKL_BLE_GATT_INVALID_HANDLE;

    /**Assign Peer Info Into BAL Peer Info*/
    memcpy(&bal_ble_peer, &peer, sizeof(BAL_BLE_PEER_INFO_T));

    /**< After do connecting one device, we will discovery all service and char. */
    return tkl_ble_gap_connect(&bal_conn_addr, &bal_scan_params, &bal_conn_params);
}

OPERATE_RET bal_ble_disconnect(CONST BAL_BLE_PEER_INFO_T peer)
{
    USHORT_T conn_handle = peer.conn_handle;

    return tkl_ble_gap_disconnect(conn_handle, TKL_BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

OPERATE_RET bal_ble_server_common_send(BAL_BLE_DATA_T *p_data)
{
    if (p_data == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (tkl_ble_common_connect_handle != TKL_BLE_GATT_INVALID_HANDLE
        && tkl_ble_common_char[BAL_COMMON_NOTIFY_CHAR_INDEX].handle != TKL_BLE_GATT_INVALID_HANDLE) {

        return tkl_ble_gatts_value_notify(tkl_ble_common_connect_handle, tkl_ble_common_char[BAL_COMMON_NOTIFY_CHAR_INDEX].handle,
            p_data->p_data, p_data->len);
    }

    return OPRT_OS_ADAPTER_BLE_NOTIFY_FAILED;
}

OPERATE_RET bal_ble_server_common_read_update(BAL_BLE_DATA_T *p_data)
{
    if (p_data == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (tkl_ble_common_connect_handle != TKL_BLE_GATT_INVALID_HANDLE
        && tkl_ble_common_char[BAL_COMMON_READ_CHAR_INDEX].handle != TKL_BLE_GATT_INVALID_HANDLE) {

        return tkl_ble_gatts_value_set(tkl_ble_common_connect_handle, tkl_ble_common_char[BAL_COMMON_READ_CHAR_INDEX].handle,
            p_data->p_data, p_data->len);
    }

    return OPRT_INVALID_PARM;
}

OPERATE_RET bal_ble_client_common_send(CONST BAL_BLE_PEER_INFO_T peer, BAL_BLE_DATA_T *p_data)
{
    if (p_data == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (peer.conn_handle != TKL_BLE_GATT_INVALID_HANDLE && peer.char_handle[BAL_COMMON_WRITE_CHAR_INDEX] != TKL_BLE_GATT_INVALID_HANDLE) {
        return tkl_ble_gattc_write_without_rsp(peer.conn_handle, peer.char_handle[BAL_COMMON_WRITE_CHAR_INDEX], p_data->p_data, p_data->len);
    }

    return OPRT_OS_ADAPTER_BLE_WRITE_FAILED;
}

OPERATE_RET bal_ble_client_common_read(CONST BAL_BLE_PEER_INFO_T peer)
{
    if (peer.conn_handle != TKL_BLE_GATT_INVALID_HANDLE && peer.char_handle[BAL_COMMON_READ_CHAR_INDEX] != TKL_BLE_GATT_INVALID_HANDLE) {
        return tkl_ble_gattc_read(peer.conn_handle, peer.char_handle[BAL_COMMON_READ_CHAR_INDEX]);
    }

    return OPRT_OS_ADAPTER_BLE_READ_FAILED;
}

OPERATE_RET bal_ble_server_exchange_mtu_reply(CONST BAL_BLE_PEER_INFO_T peer, USHORT_T server_mtu)
{
    USHORT_T conn_handle = peer.conn_handle;

    if (server_mtu < 23 || server_mtu > 247) {
        return OPRT_INVALID_PARM;
    }

    return tkl_ble_gatts_exchange_mtu_reply(conn_handle, server_mtu);
}

OPERATE_RET bal_ble_client_exchange_mtu_request(CONST BAL_BLE_PEER_INFO_T peer, USHORT_T client_mtu)
{
    USHORT_T conn_handle = peer.conn_handle;

    if (client_mtu < 23 || client_mtu > 247) {
        return OPRT_INVALID_PARM;
    }

    return tkl_ble_gattc_exchange_mtu_request(conn_handle, client_mtu);
}

OPERATE_RET bal_ble_vendor_command_control(USHORT_T opcode, VOID_T *user_data, USHORT_T data_len)
{
    return tkl_ble_vendor_command_control(opcode, user_data, data_len);
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET bal_ble_cloud_func_register(BAL_BLE_LOG_SEQ_FUNC log_seq_func, BAL_BLE_PUBLISH_CB *publish_func)
{
    return OPRT_NOT_SUPPORTED;
}
