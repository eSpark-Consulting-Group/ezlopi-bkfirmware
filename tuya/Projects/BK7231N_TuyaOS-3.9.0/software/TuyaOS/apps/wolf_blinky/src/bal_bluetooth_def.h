/**
 * @file bal_bluetooth_def.h
 * @brief This is bal_bluetooth_def file
 * @version 1.0
 * @date 2023-09-26
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __BAL_BLUETOOTH_DEF_H__
#define __BAL_BLUETOOTH_DEF_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
/**< Tuya Needed Definitions And Macro */
/* Tuya Ble Command Service UUID First-Version */
#define BAL_BLE_CMD_SERVICE_UUID_V1                     (0x1910)
/* Tuya Ble Write Characteristic UUID First-Version*/
#define BAL_BLE_CMD_WRITE_CHAR_UUID_V1                  (0x2B11)
/* Tuya Ble Notify Characteristic UUID First-Version*/
#define BAL_BLE_CMD_NOTIFY_CHAR_UUID_V1                 (0x2B10)

/* Tuya Ble Bulk Service UUID First-Version */
#define BAL_BLE_BULK_SERVICE_UUID_V1                    (0x1920)
/* Tuya Ble Write Characteristic UUID First-Version*/
#define BAL_BLE_BULK_WRITE_CHAR_UUID_V1                 (0x2B23)
/* Tuya Ble Notify Characteristic UUID First-Version*/
#define BAL_BLE_BULK_NOTIFY_CHAR_UUID_V1                (0x2B24)

#define BAL_BLE_SPECIFIC_SVC_UUID128                    {0x12, 0x19, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00}

/* Tuya Ble Command Service UUID Second-Version */
#define BAL_BLE_CMD_SERVICE_UUID_V2                     (0xFD51)
/* Tuya Ble Write Characteristic UUID Second-Version*/
#define BAL_BLE_CMD_WRITE_CHAR_UUID_V2                  (0x0001)
#define BAL_BLE_CMD_WRITE_CHAR_UUID128_V2               {0xD0, 0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}
/* Tuya Ble Notify Characteristic UUID Second-Version*/
#define BAL_BLE_CMD_NOTIFY_CHAR_UUID_V2                 (0x0002)
#define BAL_BLE_CMD_NOTIFY_CHAR_UUID128_V2              {0xD0, 0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}
/* Tuya Ble Read Characteristic UUID Second-Version*/
#define BAL_BLE_CMD_READ_CHAR_UUID_V2                   (0x0003)
#define BAL_BLE_CMD_READ_CHAR_UUID128_V2                {0xD0, 0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}

/* Tuya Ble Service UUID Scan UUID */
#define BAL_BLE_SVC_SCAN_UUID_V1                        (0xA201)
#define BAL_BLE_SVC_SCAN_UUID_V2                        (0xA300)
#define BAL_BLE_SVC_SCAN_UUID_V3                        (0xFD50)

/* Tuya Ble Service Characteristic Index */
#define BAL_COMMON_WRITE_CHAR_INDEX                     (0)
#define BAL_COMMON_NOTIFY_CHAR_INDEX                    (1)
#define BAL_COMMON_READ_CHAR_INDEX                      (2)
#define BAL_COMMON_RESERVED_INDEX                       (3)     // For Reserved Handle Index
#define BAL_MESH_OTA_WRITE_CHAR_INDEX                   (4)
#define BAL_MESH_PROXY_WRITE_CHAR_INDEX                 (5)
#define BAL_MESH_FW_READ_CHAR_INDEX                     (6)

#define DEFAULT_ADV_PARAMS(min, max)                \
    {                                               \
            .adv_interval_min = min,                \
            .adv_interval_max = max,                \
            .adv_type = BAL_BLE_ADV_TYPE_CS_UNDIR,  \
    }
#define TUYAOS_BLE_DEFAULT_ADV_PARAM ((BAL_BLE_ADV_PARAMS_T *) (&(BAL_BLE_ADV_PARAMS_T) DEFAULT_ADV_PARAMS(30, 60)))

#define DEFAULT_SCAN_PARAMS(interval, window)       \
    {                                               \
        .type = BAL_BLE_SCAN_TYPE_ACTIVE,           \
        .scan_interval = interval,                  \
        .scan_window = window,                      \
        .timeout = 0x0000,                          \
        .filter_dup = 1,                            \
    }
#define TUYAOS_BLE_DEFAULT_SCAN_PARAM ((BAL_BLE_SCAN_PARAMS_T *) (&(BAL_BLE_SCAN_PARAMS_T) DEFAULT_SCAN_PARAMS(30, 30)))

#define DEFAULT_CONN_PARAMS(min, max)               \
    {                                               \
        .min_conn_interval = min,                   \
        .max_conn_interval = max,                   \
        .latency = 0,                               \
        .conn_sup_timeout = 0x100,                  \
        .connection_timeout = 5,                    \
    }
#define TUYAOS_BLE_DEFAULT_CONN_PARAM ((BAL_BLE_CONN_PARAMS_T *) (&(BAL_BLE_CONN_PARAMS_T) DEFAULT_CONN_PARAMS(30, 60)))

typedef enum {
    BAL_BLE_ADDR_TYPE_PUBLIC                = 0x00,     /**< public address  */
    BAL_BLE_ADDR_TYPE_RANDOM                = 0x01,     /**< random address  */
} BAL_BLE_ADDR_TYPE_E;

typedef enum {
    BAL_BLE_ADV_DATA,                                   /**< Adv Data - Only */
    BAL_BLE_RSP_DATA,                                   /**< Scan Response Data - Only */
    BAL_BLE_ADV_RSP_DATA,                               /**< Adv Data + Scan Response Data */
    BAL_BLE_NONCONN_ADV_DATA,                           /**< None-Connectable Adv Data - Only */
    BAL_BLE_EXTENDED_ADV_DATA,                          /**< [Bluetooth 5.0]Extended Adv Data - Only */
} BAL_BLE_ADV_DATA_TYPE_E;

typedef enum {
    BAL_BLE_ADV_TYPE_CS_UNDIR               = 0x01,     /**< Connectable and scannable undirected advertising events. [Tuya Default Value]*/
    BAL_BLE_ADV_TYPE_CNS_DIR_HIGH_DUTY      = 0x02,     /**< Reserved  */
    BAL_BLE_ADV_TYPE_CNS_DIR                = 0x03,     /**< Reserved  */
    BAL_BLE_ADV_TYPE_NCS_UNDIR              = 0x04,     /**< Non-connectable scannable undirected advertising events. */
    BAL_BLE_ADV_TYPE_NCNS_UNDIR             = 0x05,     /**< Non-connectable non-scannable undirected advertising events. */
} BAL_BLE_ADV_TYPE_E;

/**< Define these parameters for scanning */
typedef enum {
    BAL_BLE_SCAN_TYPE_ACTIVE,                           /**< active scanning, we can scan the respond data, tuya default value */
    BAL_BLE_SCAN_TYPE_PASSIVE,                          /**< passive scanning */
} BAL_BLE_SCAN_TYPE_E;

typedef enum {
    BAL_BLE_SVC_UUID_V1         = 0x01,
    BAL_BLE_SVC_UUID_V2         = 0x02,
    BAL_BLE_SVC_UUID_MESH_OTA   = 0x04,
    BAL_BLE_SVC_UUID_MESH_VER   = 0x08,
    BAL_BLE_SVC_UUID_MESH_PROXY = 0x10,
} BAL_BLE_SVC_UUID_TYPE_E;

/**< Define these parameters for ble event */
typedef enum {
    BAL_BLE_ROLE_PERIPERAL          = 0x0001,           /**< Ble Peripheral Mode. Will Combine with Tuya Ble SDK, and do server's operations */
    BAL_BLE_ROLE_CENTRAL            = 0x0002,           /**< Ble Central Mode. Will Combine with Tuya Bluetooth Gateway SDK, and do client's operations */
    BAL_BLE_ROLE_BEACON             = 0x0004,           /**< Ble Beacon Mode, will init with ble Peripheral */

    BAL_MESH_ROLE_ADV_PROVISIONER   = 0x0100,           /**< Bluetooth Mesh Provisioner Mode, Support PB-ADV */
    BAL_MESH_ROLE_GATT_PROVISIONER  = 0x0200,           /**< Bluetooth Mesh Provisioner Mode, Support PB-GATT */
    BAL_MESH_ROLE_NODE              = 0x0400,           /**< Bluetooth Mesh Node Mode*/
} BAL_BLE_ROLE_E;

typedef enum {
    BAL_BLE_STACK_INIT = 0x01,                          /**< Successfully init ble/bt stack */

    BAL_BLE_STACK_DEINIT,                               /**< Successfully deinit ble/bt stack */

    BAL_BLE_EVT_PERIPHERAL_CONNECT,                     /**< Connected as peripheral role */

    BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY,              /**< Connected as central role, include mesh gatt, And Complete the service discovery  */

    BAL_BLE_EVT_DISCONNECT,                             /**< Disconnected */

    BAL_BLE_EVT_ADV_REPORT,                             /**< Scan result report */

    BAL_BLE_EVT_CONN_PARAM_REQ,                         /**< Parameter update request */

    BAL_BLE_EVT_CONN_PARAM_UPDATE,                      /**< Parameter update successfully */

    BAL_BLE_EVT_CONN_RSSI,                              /**< Got RSSI value of link peer device */

    BAL_BLE_EVT_MTU_REQUEST,                            /**< MTU exchange request event, For Ble peripheral, we need to do reply*/

    BAL_BLE_EVT_MTU_RSP,                                /**< MTU exchange respond event, For Ble Central, the ble central has finished the MTU-Request */

    BAL_BLE_EVT_NOTIFY_TX,                              /**< [Ble peripheral] Transfer data Callback, Will Report Success Or Fail */

    BAL_BLE_EVT_WRITE_REQ,                              /**< [Ble Peripheral] Get Client-Write Char Request */

    BAL_BLE_EVT_NOTIFY_RX,                              /**< [Ble Central] Get Notification data */

    BAL_BLE_EVT_READ_RX,                                /**< [Ble Central] Receive data from reading char */

    BAL_BLE_EVT_SUBSCRIBE,                              /**< [Ble Peripheral] Event Subscribe */

    BAL_BLE_EVT_READ_CHAR,                              /**< [Ble Peripheral] Read Char Event*/
} BAL_BLE_EVT_TYPE_E;
/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    BAL_BLE_ADDR_TYPE_E     type;                       /**< Mac Address Type, Refer to @ BAL_BLE_ADDR_TYPE_E */
    UCHAR_T                 addr[6];                    /**< GAP Address, Address size, 6 bytes */
} BAL_BLE_ADDR_T;

typedef struct {
    USHORT_T                adv_interval_min;           /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    USHORT_T                adv_interval_max;           /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    BAL_BLE_ADV_TYPE_E      adv_type;                   /**< Adv Type, Refer to @ BAL_BLE_ADV_TYPE_E */
    BAL_BLE_ADDR_T          direct_addr;                /**< For direct advertising, we need to set direct addr, otherise, fill NULL */
} BAL_BLE_ADV_PARAMS_T;

typedef struct {
    BAL_BLE_ADDR_T          peer_addr;                  /**< After Scan Adv, we can get peer mac and mac type */
    BAL_BLE_ADV_DATA_TYPE_E adv_type;                   /**< Point the advertising type, refer to @ BAL_BLE_ADV_DATA_TYPE_E */

    CHAR_T                  rssi;                       /**< After scan Adv, we can get advertising's rssi */
    UCHAR_T                 *p_data;                    /**< After scan Adv, we can get advertising's data*/
    UCHAR_T                 data_len;                   /**< advertising data length */
} BAL_BLE_ADV_REPORT_T;

typedef struct {
    USHORT_T                len;                        /**< Data length for p_data. */
    UCHAR_T                 *p_data;                    /**< Data Pointer. */
} BAL_BLE_DATA_T;

typedef struct {
    BAL_BLE_SCAN_TYPE_E type;

    USHORT_T                scan_interval;              /**< Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 sec */
    USHORT_T                scan_window;                /**< Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 seconds */
    USHORT_T                timeout;                    /**< Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. */
    UCHAR_T                 filter_dup;                 /**< Duplicate filtering ENABLE or DISABLE) */
} BAL_BLE_SCAN_PARAMS_T;

typedef struct {
    USHORT_T                uuid16;
    USHORT_T                start_handle;
    USHORT_T                end_handle;
} BAL_BLE_UUID_ARRAY_T;

/**< Define these parameters for connecting */
typedef struct {
    USHORT_T                conn_handle;
    USHORT_T                char_handle[6];             /**< Tuya Char Handle, It can reflect any characteristic, Max Characteristic refer to @TKL_BLE_GATT_CHAR_MAX_NUM*/

    BAL_BLE_ADDR_T          peer_addr;                  /**< Reserved, Only Connecetion Handle can stand for one specific device
                                                            For Some Reason on Bluetooth Gateway, we need device's address.*/
} BAL_BLE_PEER_INFO_T;

typedef struct {
    BAL_BLE_PEER_INFO_T     peer;                       /**< For each report, we will tell the host peer information. this info can be get after being connected*/
    BAL_BLE_DATA_T          report;                     /**< Report Data */
} BAL_BLE_DATA_REPORT_T;

typedef struct {
    USHORT_T                min_conn_interval;          /**< Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds. */
    USHORT_T                max_conn_interval;          /**< Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds. */
    USHORT_T                latency;                    /**< Range: 0x0000 to 0x01F3 */
    USHORT_T                conn_sup_timeout;           /**< Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 seconds */

    USHORT_T                connection_timeout;         /**< Range: 0 to 5000ms [Tuya Ble Central Use]*/
} BAL_BLE_CONN_PARAMS_T;

typedef struct {
    BAL_BLE_PEER_INFO_T     peer;                       /**< Connection handle on which the event occured.*/

    BAL_BLE_CONN_PARAMS_T   conn_param;                 /**< [Optional] After connection is established, we will get connection parameters.*/
    INT_T                   result;                     /**< Connection Result. */
} BAL_BLE_CONNECT_EVT_T;

typedef struct {
    BAL_BLE_PEER_INFO_T     peer;                       /**< Disconnection handle on which the event occured.*/
    UCHAR_T                 reason;                     /**< Disconnection Reason */
} BAL_BLE_DISCONNECT_EVT_T;

typedef struct {
    USHORT_T                conn_handle;                /**< After ble peripheral send a notification, we will get the result and connection handle */
    USHORT_T                char_handle;                /**< After ble peripheral send a notification, we will get the result and characteristic handle */

    INT_T                   result;                     /**< Notification Result */
} BAL_BLE_NOTIFY_TX_EVT_T;

typedef struct {
    USHORT_T                conn_handle;                /**< After try to do get_rssi, we will get connection handle */
    CHAR_T                  rssi;                       /**< After try to do get_rssi, we will get rssi */
} BAL_BLE_CONN_RSSI_EVT_T;

typedef struct {
    USHORT_T                conn_handle;                /**< After try to do exchange mtu, we will get connection handle */
    USHORT_T                mtu;                        /**< Get the mtu */
} BAL_BLE_EXCHANGE_MTU_EVT_T;

typedef struct {
    USHORT_T                conn_handle;                /**< Connection Handle */
    BAL_BLE_CONN_PARAMS_T   conn;                       /**< Connection Parameters @ BAL_BLE_CONN_PARAMS_T  */
} BAL_BLE_CONN_PARAM_EVT_T;

typedef struct {
    USHORT_T                        conn_handle;
    USHORT_T                        char_handle;                /**< Specify one characteristic handle */
    UCHAR_T                         reason;
    UCHAR_T                         prev_notify     :    1;     /**< previously subscribed */
    UCHAR_T                         cur_notify      :    1;     /** currently notifications. */
    UCHAR_T                         prev_indicate   :    1;     /** previously indications. */
    UCHAR_T                         cur_indicate    :    1;     /** currently subscribed to indications. */
} BAL_BLE_SUBSCRBE_EVT_T;

typedef struct {
    USHORT_T                        conn_handle;
    USHORT_T                        char_handle;                /**< Specify one characteristic handle */
    USHORT_T                        offset;                     /**< Char Offset */
} BAL_BLE_READ_CHAR_EVT_T;

typedef struct {
    BAL_BLE_EVT_TYPE_E              type;

    union {
        UCHAR_T                     init;               /**< Show init states */
        BAL_BLE_CONNECT_EVT_T       connect;            /**< Receive connect callback, This value can be used with BAL_BLE_EVT_PERIPHERAL_CONNECT and BAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY */
        BAL_BLE_DISCONNECT_EVT_T    disconnect;         /**< Receive disconnect callback */
        BAL_BLE_ADV_REPORT_T        adv_report;         /**< Receive Adv and Respond report */
        BAL_BLE_CONN_PARAM_EVT_T    conn_param;         /**< We will update connect parameters.This value can be used with BAL_BLE_EVT_CONN_PARAM_REQ and BAL_BLE_EVT_CONN_PARAM_UPDATE */
        BAL_BLE_EXCHANGE_MTU_EVT_T  exchange_mtu;       /**< This value can be used with BAL_BLE_EVT_MTU_REQUEST and BAL_BLE_EVT_MTU_RSP */
        BAL_BLE_CONN_RSSI_EVT_T     link_rssi;          /**< Peer device RSSI value */
        BAL_BLE_NOTIFY_TX_EVT_T     notify_result;      /**< [Ble Peripheral] This value can be used with BAL_BLE_EVT_NOTIFY_TX after Ble Peripheral send a notification to peer. */
        BAL_BLE_DATA_REPORT_T       write_report;       /**< This value can be used with BAL_BLE_EVT_WRITE_REQ */
        BAL_BLE_DATA_REPORT_T       data_report;        /**< This value can be used with BAL_BLE_EVT_NOTIFY_RX */
        BAL_BLE_DATA_REPORT_T       data_read;          /**< After we do read attr in central mode, we will get the callback from bluetooth Kernel */
        BAL_BLE_SUBSCRBE_EVT_T      subscribe;          /**< used with BAL_BLE_EVT_SUBSCRIBE*/
        BAL_BLE_READ_CHAR_EVT_T     char_read;          /**< read char event, used with BAL_BLE_EVT_READ_CHAR*/
    }ble_event;
} BAL_BLE_EVT_PARAMS_T;

/**< Define Gap Event Callback for ble peripheral and central */
typedef VOID(*BAL_BLE_EVT_FUNC_CB)(BAL_BLE_EVT_PARAMS_T *p_event);

/**< Define Tuya Internal Log Sequence Function */
typedef VOID(*BAL_BLE_LOG_SEQ_FUNC)(UCHAR_T log_id, VOID_T* data);

/**< Define Tuya Internal Log Sequence Function */
typedef VOID(*BAL_BLE_PUBLISH_CB)(UCHAR_T publish_id, VOID_T* data);

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* __BAL_BLUETOOTH_DEF_H__ */