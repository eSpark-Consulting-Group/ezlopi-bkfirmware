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
#include "tal_bluetooth.h"

#include "tkl_bluetooth.h"
#include "tkl_bluetooth_def.h"
#include "tal_bluetooth_def.h"
#include "tal_bluetooth_ext.h"


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

#include "esp_partition.h"

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26


/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);
uint32_t __ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
void flash_read_write_test(void);
VOID example_kv();

/* Thread Handles */

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE bk_rcv_thread = NULL;
STATIC THREAD_HANDLE free_heap_thrd = NULL;

#if 0
void test_nvs_string_operations() {
    const char* key = "test_key";
    const char* value_to_write = "test_value";

    // Write the string to NVS
    esp_err_t write_result = nvs_set_str(NULL, key, value_to_write);
    if (write_result == ESP_OK) {
        TRACE_I("String written OPRT_OKfully.\n");
    } else {
        TRACE_I("Failed to write string. Error code: %d\n", write_result);
        return;
    }

    // Read the string from NVS
    size_t required_length = 0;
    esp_err_t read_result = nvs_get_str(NULL, key, NULL, &required_length);
    if (read_result == ESP_OK) {
        char* read_value = (char*)malloc(required_length);
        if (read_value == NULL) {
            TRACE_I("Failed to allocate memory for reading string.\n");
            return;
        }

        read_result = nvs_get_str(NULL, key, read_value, &required_length);
        if (read_result == ESP_OK) {
            TRACE_I("String read OPRT_OKfully: %s\n", read_value);
        } else {
            TRACE_I("Failed to read string. Error code: %d\n", read_result);
        }

        free(read_value);
    } else if (read_result == ESP_ERR_NVS_NOT_FOUND) {
        TRACE_I("Key not found.\n");
    } else {
        TRACE_I("Failed to read string length. Error code: %d\n", read_result);
    }
}
#endif

#if 0
void test_nvs_apis() {
    esp_err_t err;
    
    // Test int8_t
    int8_t i8_write = -128, i8_read = 0;
    err = nvs_set_i8(NULL, "i8_key", i8_write);
    if (err == ESP_OK) {
        err = nvs_get_i8(NULL, "i8_key", &i8_read);
        TRACE_W("int8_t: written = %d, read = %d, status = %s\n", i8_write, i8_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test uint8_t
    uint8_t u8_write = 255, u8_read = 0;
    err = nvs_set_u8(NULL, "u8_key", u8_write);
    if (err == ESP_OK) {
        err = nvs_get_u8(NULL, "u8_key", &u8_read);
        TRACE_W("uint8_t: written = %u, read = %u, status = %s\n", u8_write, u8_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test int16_t
    int16_t i16_write = -32768, i16_read = 0;
    err = nvs_set_i16(NULL, "i16_key", i16_write);
    if (err == ESP_OK) {
        err = nvs_get_i16(NULL, "i16_key", &i16_read);
        TRACE_W("int16_t: written = %d, read = %d, status = %s\n", i16_write, i16_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test uint16_t
    uint16_t u16_write = 65535, u16_read = 0;
    err = nvs_set_u16(NULL, "u16_key", u16_write);
    if (err == ESP_OK) {
        err = nvs_get_u16(NULL, "u16_key", &u16_read);
        TRACE_W("uint16_t: written = %u, read = %u, status = %s\n", u16_write, u16_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test int32_t
    int32_t i32_write = -2147483648, i32_read = 0;
    err = nvs_set_i32(NULL, "i32_key", i32_write);
    if (err == ESP_OK) {
        err = nvs_get_i32(NULL, "i32_key", &i32_read);
        TRACE_W("int32_t: written = %d, read = %d, status = %s\n", i32_write, i32_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test uint32_t
    uint32_t u32_write = 4294967295, u32_read = 0;
    err = nvs_set_u32(NULL, "u32_key", u32_write);
    if (err == ESP_OK) {
        err = nvs_get_u32(NULL, "u32_key", &u32_read);
        TRACE_W("uint32_t: written = %u, read = %u, status = %s\n", u32_write, u32_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    #if 0
    // Test int64_t
    int64_t i64_write = -9223372036854775807LL, i64_read = 0;
    err = nvs_set_i64(NULL, "i64_key", i64_write);
    if (err == ESP_OK) {
        err = nvs_get_i64(NULL, "i64_key", &i64_read);
        TRACE_W("int64_t: written = %lld, read = %lld, status = %s\n", i64_write, i64_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test uint64_t
    uint64_t u64_write = 18446744073709551615ULL, u64_read = 0;
    err = nvs_set_u64(NULL, "u64_key", u64_write);
    if (err == ESP_OK) {
        err = nvs_get_u64(NULL, "u64_key", &u64_read);
        TRACE_W("uint64_t: written = %llu, read = %llu, status = %s\n", u64_write, u64_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }
    #endif

    // Test string
    const char *str_write = "Hello, NVS!";
    char str_read[20] = {0};
    size_t str_len = sizeof(str_read);
    err = nvs_set_str(NULL, "str_key", str_write);
    if (err == ESP_OK) {
        err = nvs_get_str(NULL, "str_key", str_read, &str_len);
        TRACE_W("string: written = %s, read = %s, status = %s\n", str_write, str_read, err == ESP_OK ? "OPRT_OK" : "FAIL");
    }

    // Test blob
    uint8_t blob_write[5] = {1, 2, 3, 4, 5};
    uint8_t blob_read[5] = {0};
    size_t blob_len = sizeof(blob_read);
    err = nvs_set_blob(NULL, "blob_key", blob_write, sizeof(blob_write));
    if (err == ESP_OK) {
        err = nvs_get_blob(NULL, "blob_key", blob_read, &blob_len);
        TRACE_W("blob: written = {1, 2, 3, 4, 5}, read = {%u, %u, %u, %u, %u}, status = %s\n",
               blob_read[0], blob_read[1], blob_read[2], blob_read[3], blob_read[4],
               err == ESP_OK ? "OPRT_OK" : "FAIL");
    }
}
#endif

void dummy_world()
{
    TRACE_W("Dummy World");
}
#define TAL_BLE_EX
#ifdef TAL_BLE_EX
/***********************************************************
***********************variable define**********************
***********************************************************/
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

STATIC char ble_rx_buffer[2 * 1024] = {0};

STATIC TAL_BLE_PEER_INFO_T sg_ble_peripheral_info;

/***********************************************************
***********************function define**********************
***********************************************************/
/**
* @brief bluebooth event callback function
*
* @param[in] p_event: bluebooth event
* @return none
*/
STATIC VOID_T __ble_peripheral_event_callback(TAL_BLE_EVT_PARAMS_T *p_event)
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
                tal_ble_advertising_data_set(&adv_data, &rsp_data);
                tal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
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
                tal_ble_server_common_read_update(&read_data);
            } else {
                memset(&sg_ble_peripheral_info, 0, sizeof(TAL_BLE_PEER_INFO_T));
            }
            break;
        }
        case TAL_BLE_EVT_DISCONNECT: {
            TAL_PR_DEBUG("ble_peripheral disconnect.");
            tal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
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
            }
            break;
        }
        case TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY:
            {
                TAL_PR_DEBUG("EVENT = TAL_BLE_EVT_CENTRAL_CONNECT_DISCOVERY");
            }
            break;

        default: break;
    }
}


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
                bal_ble_advertising_data_set(&adv_data, &rsp_data);
                bal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
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
                bal_ble_server_common_read_update(&read_data);
            } else {
                memset(&sg_ble_peripheral_info, 0, sizeof(TAL_BLE_PEER_INFO_T));
            }
            break;
        }
        case TAL_BLE_EVT_DISCONNECT: {
            TAL_PR_DEBUG("ble_peripheral disconnect.");
            bal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
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
                bal_ble_conn_param_update(peer_info, &conn_param);
            }
            break;

        default: break;
    }
}


#endif

#define TKL_BLE_EX

#ifdef TKL_BLE_EX
// STATIC UCHAR_T adv_data_const[31] =
// {
//     0x02,
//     0x01,
//     0x06,
//     0x03,
//     0x02,
//     0x50, 0xFD,
//     0x17,
//     0x16,
//     0x50, 0xFD,
//     0x41, 0x00,       //Frame Control
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };

// STATIC UCHAR_T scan_rsp_data_const[31] =
// {   
//     0x17,             // length
//     0xFF,
//     0xD0,
//     0x07,
//     0x00, //Encry Mode(8)
//     0x00,0x00, //communication way bit0-mesh bit1-wifi bit2-zigbee bit3-NB
//     0x00, //FLAG
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
// 	0x05, //24
//     0x09,
//     0x4D, 0x61, 0x61, 0x7a
// };

void gap_evt_handler(TKL_BLE_GAP_PARAMS_EVT_T *evt_data) {
    TKL_BLE_GAP_CONNECT_EVT_T conn_evt;
    switch (evt_data->type) {
        case TKL_BLE_GAP_EVT_CONNECT:
            // Handle connection event
            conn_evt = evt_data->gap_event.connect;
            TAL_PR_DEBUG("Connected to peer, handle: %d\n", conn_evt.conn_params.connection_timeout);
            TAL_PR_DEBUG("Device connected: %d\n", evt_data->conn_handle);
            break;
        case TKL_BLE_GAP_EVT_DISCONNECT:
            // Handle disconnection event
            TAL_PR_DEBUG("Device disconnected: %d\n", evt_data->conn_handle);
            break;
        default:
            break;
    }
}

void gatt_evt_handler(TKL_BLE_GATT_PARAMS_EVT_T *evt_data) {
    TAL_PR_DEBUG("GATT_EVENT %d", evt_data->type);
    switch (evt_data->type) {
        case TKL_BLE_GATT_EVT_READ_CHAR_VALUE:
            // Handle read request
            {
                USHORT_T char_handle = evt_data->gatt_event.char_read.char_handle;
                // Retrieve the characteristic value and send it back
                UCHAR_T value[20]; // Example buffer, adjust as needed
                USHORT_T length = sizeof(value);
                
                // Fill value based on characteristic
                tkl_ble_gatts_value_get(evt_data->conn_handle, char_handle, value, length);
                
                // Respond to the read request
                tkl_ble_gatts_value_notify(evt_data->conn_handle, char_handle, value, length);
            }
            break;
        case TKL_BLE_GATT_EVT_WRITE_REQ:
            // Handle write request
            {
                USHORT_T char_handle = evt_data->gatt_event.char_read.char_handle;
                UCHAR_T *data = evt_data->gatt_event.write_report.report.p_data;
                USHORT_T length = evt_data->gatt_event.write_report.report.length;

                // Process the received data as needed
                TAL_PR_DEBUG("Received data on write: %.*s\n", length, data);
                
                // Optionally send a response if needed
                tkl_ble_gatts_value_set(evt_data->conn_handle, char_handle, data, length);
            }
            break;
        case TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY:
            {
                TAL_PR_DEBUG("EVENT = TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY");
            }
            break;
        case TKL_BLE_GATT_EVT_CHAR_DISCOVERY:
            {
                TAL_PR_DEBUG("EVENT = TKL_BLE_GATT_EVT_CHAR_DISCOVERY");
            }   
            break;
        default:
            break;
    }
}

// Define the UUIDs for the service and characteristics (replace with your actual UUIDs)
    TKL_BLE_UUID_T service_uuid = {
        .uuid_type = TKL_BLE_UUID_TYPE_16,              // Specify the UUID type
        .uuid.uuid16 = 0x1800,                           // Set the 16-bit UUID value
    };
    TKL_BLE_UUID_T char_uuid1 = {
        .uuid_type = TKL_BLE_UUID_TYPE_16,
        .uuid.uuid16 = 0x2A37, // Correct UUID for Heart Rate Measurement
    };

    TKL_BLE_UUID_T char_uuid2 = {
        .uuid_type = TKL_BLE_UUID_TYPE_16,
        .uuid.uuid16 = 0x2A38, // Correct UUID for Body Sensor Location
    };

    // Define the characteristics with their permissions and properties
    TKL_BLE_CHAR_PARAMS_T char_params[2] = {
        {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,                           // This will be set by the BLE stack
            .char_uuid = {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = TAL_BLE_CMD_WRITE_CHAR_UUID_V1, // Correct UUID for Heart Rate Measurement
                },               // UUID for the first characteristic
            .property = TKL_BLE_GATT_CHAR_PROP_WRITE | TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP, // Example properties
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE, // Permissions
            .value_len = 244,                       // Length of the characteristic value
        },
        {
            .handle = TKL_BLE_GATT_INVALID_HANDLE,                           // This will be set by the BLE stack
            .char_uuid = {
                    .uuid_type = TKL_BLE_UUID_TYPE_16,
                    .uuid.uuid16 = TAL_BLE_CMD_NOTIFY_CHAR_UUID_V1, // Correct UUID for Body Sensor Location
                },               // UUID for the second characteristic
            .property = TKL_BLE_GATT_CHAR_PROP_NOTIFY,    // Example property
            .permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE,   // Permissions
            .value_len = 244,                       // Length of the characteristic value
        }
    };
    TKL_BLE_SERVICE_PARAMS_T service1 = {
        .handle = TKL_BLE_GATT_INVALID_HANDLE,                               // This will be set by the BLE stack
        .svc_uuid = {
                .uuid_type = TKL_BLE_UUID_TYPE_16,              // Specify the UUID type
                .uuid.uuid16 = 0xfd50,                           // Set the 16-bit UUID value
            },                  // UUID for the service
        .type = TKL_BLE_UUID_SERVICE_PRIMARY,      // Service type (primary or secondary)
        .char_num = 2, // Number of characteristics
        .p_char = char_params                       // Pointer to the characteristics
    };
    TKL_BLE_GATTS_PARAMS_T gatt_service = { 
        .svc_num = 1,
        .p_service = &service1
    };

    TKL_BLE_GAP_ADV_PARAMS_T adv_params = {
        .adv_type = TKL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED,  // Set the advertising type
        // .direct_addr = {0},                                            // Fill in direct address if needed (e.g., set to 0 for non-directed advertising)
        .adv_interval_min = 30,                                   // Minimum advertising interval (20 ms)
        .adv_interval_max = 60,                                   // Maximum advertising interval (10.24 sec)
        // .adv_channel_map = 0x07,                                     // Advertising on all three channels (37, 38, 39)
    };


void setup_ble_peripheral() {
    // Initialize BLE stack as peripheral
    tkl_ble_stack_init(TKL_BLE_ROLE_SERVER);
    
    // Add services
    int result = tkl_ble_gatts_service_add(&gatt_service);
    if (result != OPRT_OK) {
        TAL_PR_ERR("Failed to add service, error code: %d\n", result);
    }
    // tkl_ble_gatts_service_add(&service2);

    // Start advertising
    TKL_BLE_DATA_T adv_data;
    TKL_BLE_DATA_T rsp_data;

    adv_data.p_data = adv_data_const;
    adv_data.length = sizeof(adv_data_const);
    rsp_data.p_data = scan_rsp_data_const;
    rsp_data.length = sizeof(scan_rsp_data_const);

    tkl_ble_gap_adv_rsp_data_set(&adv_data, &rsp_data);

    tkl_ble_gap_adv_start(&adv_params);

    // Register callbacks
    tkl_ble_gap_callback_register(gap_evt_handler);
    tkl_ble_gatt_callback_register(gatt_evt_handler);

    while(1)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        TAL_PR_DEBUG("Running!");
        // tkl_ble_gatts_value_get(0, 0)
    }
}

#endif

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

    // TaskHandle_t free_heap_task_handle = NULL;
    // xTaskCreate(__free_heap_task, "freeheap-service", 2048, NULL, 2, &free_heap_task_handle);

    ezlopi_core_set_log_upcalls();

    // ezlopi_init();

    // ezlopi_service_broadcast_init();

    // ezlopi_service_web_provisioning_init();

    // ezlopi_nvs_set_boot_count(ezlopi_system_info_get_boot_count() + 1);

    


    #ifdef TAL_BLE_EX
    // TUYA_CALL_ERR_LOG(tal_ble_bt_init(TAL_BLE_ROLE_PERIPERAL, __ble_peripheral_event_callback));
    #endif 
    // setup_ble_peripheral();

    TUYA_CALL_ERR_LOG(tal_ble_ext_bt_init(TAL_BLE_ROLE_PERIPERAL, ___bal_ble_peripheral_event_callback));
    // TUYA_CALL_ERR_LOG(bal_ble_bt_init(TAL_BLE_ROLE_PERIPERAL, __ble_peripheral_event_callback));

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
    // tal_system_sleep(30000);

    // ezlopi_service_loop_init();
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

#if 0
uint32_t ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
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
    //     TRACE_I("Flash Write OPRT_OK");
    // } else {
    //     TRACE_E("Flash Write Failed");
    //     return;
    // }

    // Read from flash
    if (bk_flash_read(BK_PARTITION_PROV_DATA, EZLOPI_FINFO_OFFSET_HUB_DATA + EZLOPI_FINFO_LEN_WIFI_SSID, outBuffer, 96) == 0) {
        TRACE_I("Flash Read OPRT_OK");
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

// #define TEST_KV_DB
#if defined TEST_KV_DB
/**
* @brief kv_common task
*
* @param[in] param:Task parameters
* @return none
*/
#define KEY_NAME "my_key"

VOID example_kv()
{
    OPERATE_RET rt = OPRT_OK;
    INT_T i;
    BYTE_T  *read_buf = NULL;
    UINT_T read_len;
    BOOL_T is_exist = FALSE;

    TAL_PR_NOTICE("------ kv common example start ------");

    /* Write "my_key" value to kv database */
    BYTE_T write_buf[] = {0x00, 0x01, 0x02, 0x03};

    TUYA_CALL_ERR_GOTO(wd_common_write(KEY_NAME, write_buf, CNTSOF(write_buf)), __EXIT);

    /* Output the values written to the KV database */
    TAL_PR_NOTICE("kv flash write data:");
    for (i=0; i<CNTSOF(write_buf); i++) {
        TAL_PR_DEBUG_RAW("0x%02x ", write_buf[i]);
    }
    TAL_PR_DEBUG_RAW("\r\n\r\n");

    /* Check whether "my_key" exists in the kv database */
    TUYA_CALL_ERR_GOTO(wd_common_exist(KEY_NAME, &is_exist), __EXIT);
    if (FALSE == is_exist) {
        TAL_PR_NOTICE("my_key is not exist in the kv database");
        goto __EXIT;
    }

    /* read "my_key" value from kv database */
    TUYA_CALL_ERR_GOTO(wd_common_read(KEY_NAME, &read_buf, &read_len), __EXIT);

    TAL_PR_NOTICE("read len %d, read data:", read_len);
    for ( i = 0; i < read_len; i++) {
        TAL_PR_DEBUG_RAW("0x%02x ", read_buf[i]);
    }
    TAL_PR_DEBUG_RAW("\r\n\r\n");

__EXIT:
    /*free data*/
    if (NULL != read_buf) {
        TUYA_CALL_ERR_LOG(wd_common_free_data(read_buf));
        read_buf = NULL;
    }

    /*delete key*/
    rt = wd_common_delete(KEY_NAME);
    if(OPRT_OK != rt) {
        TAL_PR_ERR("err<%d>,delete fail!", rt);
    } else {
        TAL_PR_NOTICE("my_key is deleted in kv database");
    }

    return;
}
#endif //TEST_KV_DB