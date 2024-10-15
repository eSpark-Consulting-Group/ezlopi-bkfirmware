#include "ezlopi_core_ble_config.h"
#include "tal_ext_bluetooth.h"
#include "ezlopi_util_trace.h"

/**< Use TKL Definitions */
static TKL_BLE_SERVICE_PARAMS_T *ptkl_ble_service = NULL;
static TKL_BLE_CHAR_PARAMS_T *ptkl_ble_service_char = NULL;
static pv_svc_count = 0;
static pv_chr_count= 0;

void ezlopi_core_ble_gatt_service_create(TKL_BLE_UUID_T *uuid)
{
    if (!ptkl_ble_service) {
        ptkl_ble_service = tal_malloc(sizeof(TKL_BLE_SERVICE_PARAMS_T) * TKL_BLE_GATT_SERVICE_MAX_NUM);
    }

    *(ptkl_ble_service + pv_svc_count) = (TKL_BLE_SERVICE_PARAMS_T){
        .handle = TKL_BLE_GATT_INVALID_HANDLE,
        .svc_uuid = *uuid,
        .type = TKL_BLE_UUID_SERVICE_PRIMARY,
        .char_num = 0,
        .p_char = NULL,
    };

    pv_svc_count++;


    return;
}

TKL_BLE_CHAR_PARAMS_T* ezlopi_core_ble_gatt_char_create(TKL_BLE_UUID_T *uuid, UCHAR_T _property, UCHAR_T _permission, UCHAR_T _value_len)
{
    if (!ptkl_ble_service_char) {
        ptkl_ble_service_char = tal_malloc(sizeof(TKL_BLE_CHAR_PARAMS_T) * TKL_BLE_GATT_CHAR_MAX_NUM);
    }

    /**< Add Write Characteristic */
    *(ptkl_ble_service_char + pv_chr_count) = (TKL_BLE_CHAR_PARAMS_T){
        .handle = TKL_BLE_GATT_INVALID_HANDLE,
        .char_uuid = *uuid,
        .property = _property,
        .permission = _permission,
        .value_len = _value_len,
    };

    pv_chr_count++;
    
    return ptkl_ble_service_char;
}

void ezlopi_core_ble_gatt_add_characteristic(TKL_BLE_CHAR_PARAMS_T *characterstic, uint8_t num_char)
{
    static uint8_t reg_service_count = 0;
    static uint8_t reg_char_count = 0;

    (ptkl_ble_service + reg_service_count)->char_num = num_char;
    (ptkl_ble_service + reg_service_count)->p_char = characterstic;

    reg_char_count += num_char;
    reg_service_count++;
    
    return;
}


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

                {
                    // Assuming the array has 6 elements as per your structure definition
                    TAL_PR_DEBUG("Characteristic Handles:\n");
                    for (int i = 0; i < 6; i++) {
                        TAL_PR_INFO("char_handle[%d]: %u\n", i, peer_info.char_handle[i]);
                    }
                }

                {
                    TAL_PR_DEBUG("char handle is 1 = %d", ptkl_ble_service_char->handle);
                    TAL_PR_DEBUG("char handle is 2 = %d", (ptkl_ble_service_char+1)->handle);
                    TAL_PR_DEBUG("char handle is 3 = %d", (ptkl_ble_service_char+2)->handle);
                    TAL_PR_DEBUG("char handle is 4 = %d", (ptkl_ble_service_char+3)->handle);
                }

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

void ezlopi_core_ble_config_init()
{
    if (!ptkl_ble_service) {
        TRACE_D("ptkle_ble_service malloc");
        ptkl_ble_service = tal_malloc(sizeof(TKL_BLE_SERVICE_PARAMS_T) * TKL_BLE_GATT_SERVICE_MAX_NUM);
    }
    
    if (!ptkl_ble_service_char) {
        TRACE_D("ptkle_ble_service malloc");
        ptkl_ble_service_char = tal_malloc(sizeof(TKL_BLE_CHAR_PARAMS_T) * TKL_BLE_GATT_CHAR_MAX_NUM);
    }

    tal_ext_set_service_char_ptr((void*)ptkl_ble_service, (void*)ptkl_ble_service_char);
    TRACE_D("%s", __FUNCTION__);
    OPERATE_RET ret = tal_ext_ble_bt_init(TAL_BLE_ROLE_PERIPERAL, ___bal_ble_peripheral_event_callback);
    if(ret != OPRT_OK)
    {
        TRACE_E("BLE FAILED TO INIT with return %d", ret);
    }
}

TKL_BLE_SERVICE_PARAMS_T* ezlopi_core_ble_get_svc_ptr()
{
    return ptkl_ble_service;
}

TKL_BLE_CHAR_PARAMS_T* ezlopi_core_ble_get_chr_ptr()
{
    return ptkl_ble_service_char;
}