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

//these packet is heavily modified check tuya ble example for original packet
STATIC UCHAR_T adv_data_const[21] =
{
    0x02,
    0x01,
    0x06,
};

STATIC UCHAR_T scan_rsp_data_const[31] =
{   
	0x11, // this is the length of the next packet
    0x09,  //this tells what this packet is about, this the the device name shown as ble device
    0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 //and this is the name information
};

write_upcall_t provisioning_write_upcall = NULL;
write_upcall_t wifi_write_upcall = NULL;
write_upcall_t reset_write_upcall = NULL;

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
            UINT16_T char_handle = p_event->ble_event.write_report.peer.char_handle[BLE_WIFI_PROV_SERVICE_INDEX];



            for ( i = 0; i < p_event->ble_event.write_report.report.len; i++) {
                // TAL_PR_DEBUG("devicr send  data[%d]: %d", i, p_event->ble_event.write_report.report.p_data[i]);
                ble_rx_buffer[i] = (char)p_event->ble_event.write_report.report.p_data[i];
            }
            ble_rx_buffer[p_event->ble_event.write_report.report.len+1] = '\0';

            if (char_handle == sg_ble_peripheral_info.char_handle[BLE_PROV_CHAR_INDEX]) {
                TAL_PR_DEBUG("Write request on 1 characteristic");
            } 
            else if (char_handle == sg_ble_peripheral_info.char_handle[BLE_WIFI_CHAR_INDEX]) {
                TAL_PR_DEBUG("Write request on 2 characteristic");
                wifi_write_upcall(ble_rx_buffer, p_event->ble_event.write_report.report.len);
            }
            else if( char_handle == sg_ble_peripheral_info.char_handle[BLE_RESET_CHAR_INDEX]) {
                TAL_PR_DEBUG("Write request on 3 characteristic");
            }

            // TAL_PR_DEBUG("Recieved >>>>>>>>>> \n %s", ble_rx_buffer);
            // memset(ble_rx_buffer, 0, sizeof(ble_rx_buffer));
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
                UINT16_T char_handle = peer_info.char_handle[BLE_DEVICE_INFO_SERIVCE_INDEX]; 
                {
                    // Assuming the array has 6 elements as per your structure definition
                    TAL_PR_DEBUG("Characteristic Handles:\n");
                    for (int i = 0; i < 6; i++) {
                        TAL_PR_INFO("char_handle[%d]: %u\n", i, peer_info.char_handle[i]);
                    }
                }

                {
                    TAL_PR_DEBUG("tal ble evt read char");
                    for(int i = 0; i < 4; i++)
                    {
                        TAL_PR_DEBUG("sg %d", sg_ble_peripheral_info.char_handle[i]);
                    }
                }

                if(char_handle == sg_ble_peripheral_info.char_handle[BLE_DEVICE_INFO_READ_CHAR_INDEX])
                {
                    TAL_PR_DEBUG("READ request on 4 characteristic");
                }

                char data[] = "{\"device_info\":{\"firmware_version\":\"1.0.0\",\"firmware_build\":169,\"chip\":\"bk7231n\",\"uptime\":113,\"build_date\":1690548178,\"provisioned_status\":true,\"mac\":\"11:22:33:44:55:66\",\"serial\":105200426,\"ezlopi_device_type\":\"ezlopi_generic\",\"wifi_ssid\":\"eSpark Wifi\",\"wifi-connection_status\":1,\"internet_status\":1},\"api_version_info\":{\"api\":\"1.0.0\"},\"ezlopi_firmware_info\":{\"version\":\"3.4.7\",\"build\":185,\"build_date\":\"2024-02-29T18:46:33+545\"},\"mcu_chip_info\":{\"type\":\"BEKEN\",\"version\":\"1.00\"},\"mcu_halsdk_info\":{\"name\":\"TuyaOS\",\"version\":\"v3.9.0\"},\"ezlopi_current_device_status\":{\"uptime\":\"0d 0h 1m 51s\",\"boot_count\":13,\"boot_reason\":\"Power ON\",\"flash_size\":\"2MB\",\"wifi_mac\":\"EC:62:60:A7:1D:98\",\"ble_mac\":\"C8:47:8C:00:00:01\"},\"Serial_port_config\":{\"baud\":115200,\"parity\":\"N\",\"start_bits\":0,\"stop_bits\":1,\"frame_size\":8,\"flow_control\":\"DISABLED\"},\"ezlopi_cloud_info\":{\"provisioned\":true,\"uuid\":\"1b576400-8392-11ee-8889-0f56091e7639\",\"uuid_prov\":\"c2ea484b-6b31-446e-b42e-8ab7b5c36ce4\",\"type\":\"ezlopi_generic\",\"serial\":102002167,\"mac\":\"a2:f8:75:1c:3a:4c\"},\"oem_info\":{\"brand\":\"\",\"manufacturer\":\"\",\"model\":\"\"},\"network_info\":{\"ssid\":\"eSpark Wifi\",\"wifi_mode\":\"STA\",\"ip_sta\":\"192.168.18.194\",\"ip_nmask\":\"255.255.255.0\",\"ip_gw\":\"192.168.18.1\",\"wifi\":true,\"internet\":true,\"cloud\":true}}";
                data_report.p_data = data;
                data_report.len = strlen(data);
                TAL_PR_DEBUG("data = %s, len = %d", data_report.p_data, data_report.len);
                tal_ext_ble_server_common_read_update_ext(BLE_DEVICE_INFO_READ_CHAR_INDEX, &data_report);
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

void ezlopi_core_ble_set_device_name(char* ble_device_name)
{
    TRACE_D("BLE DEVICE NAME : %s", ble_device_name);
    scan_rsp_data_const[0] = 0x19;
    scan_rsp_data_const[1] = 0x09;

    memcpy(&scan_rsp_data_const[2], ble_device_name, 0x18);
}

void ezlopi_set_callback(write_upcall_t cb)
{
    wifi_write_upcall = cb;
}

TKL_BLE_SERVICE_PARAMS_T* ezlopi_core_ble_get_svc_ptr()
{
    return ptkl_ble_service;
}

TKL_BLE_CHAR_PARAMS_T* ezlopi_core_ble_get_chr_ptr()
{
    return ptkl_ble_service_char;
}