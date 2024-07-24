#include "tal_log.h"
#include "tuya_cloud_types.h"
#include "ezlopi_core_wifi.h"

bool is_connected = false;

STATIC VOID_T wifi_event_callback(WF_EVENT_E event, VOID_T *arg)
{
    OPERATE_RET op_ret = OPRT_OK;
    NW_IP_S sta_info;
    memset(&sta_info, 0, SIZEOF(NW_IP_S));

    TAL_PR_DEBUG("-------------event callback-------------");
    switch (event) {
        case WFE_CONNECTED:{
            is_connected = true;
            TAL_PR_DEBUG("connection succeeded!");

            /* output ip information */
            op_ret = tal_wifi_get_ip(WF_STATION, &sta_info);
            if (OPRT_OK != op_ret) {
                TAL_PR_ERR("get station ip error");
                return;
            }

#ifdef nwipstr                
            if(IS_NW_IPV4_ADDR(&sta_info)) {
                TAL_PR_NOTICE("gw: %s", sta_info.nwgwstr);
                TAL_PR_NOTICE("ip: %s", sta_info.nwipstr);
                TAL_PR_NOTICE("mask: %s", sta_info.nwmaskstr);
            }else {
                TAL_PR_NOTICE("ip: %s", sta_info.addr.ip6.ip);
                TAL_PR_NOTICE("islinklocal: %d", sta_info.addr.ip6.islinklocal);
            }
#else 
            TAL_PR_NOTICE("gw: %s", sta_info.gw);
            TAL_PR_NOTICE("ip: %s", sta_info.ip);
            TAL_PR_NOTICE("mask: %s", sta_info.mask);
#endif
            break;
        }

        case WFE_CONNECT_FAILED: {
            TAL_PR_DEBUG("connection fail!");
            break;
        }

        case WFE_DISCONNECTED: {
            TAL_PR_DEBUG("disconnect!");
            break;        
        }
    }
}

// VOID example_wifi_sta(INT_T argc, CHAR_T *argv[])
// {
//     OPERATE_RET rt = OPRT_OK;
//     CHAR_T connect_ssid[] = "eSpark Wifi";    // connect wifi ssid
//     CHAR_T connect_passwd[] = "47356880";   // connect wifi password

//     TAL_PR_NOTICE("------ wifi station example start ------");

//     /*WiFi init*/
//     TUYA_CALL_ERR_GOTO(tal_wifi_init(wifi_event_callback), __EXIT);

//     /*Set WiFi mode to station*/
//     TUYA_CALL_ERR_GOTO(tal_wifi_set_work_mode(WWM_STATION), __EXIT);

//     /*STA mode, connect to WiFi*/
//     TAL_PR_NOTICE("\r\nconnect wifi ssid: %s, password: %s\r\n", connect_ssid, connect_passwd);
//     TUYA_CALL_ERR_LOG(tal_wifi_station_connect((SCHAR_T *)connect_ssid, (SCHAR_T *)connect_passwd));

// __EXIT:
//     return;
// }

VOID ezlopi_wifi_initialize(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    s_ezlopi_wifi_ap_itf_t *wifi_interface = (s_ezlopi_wifi_ap_itf_t *)malloc(sizeof(s_ezlopi_wifi_ap_itf_t));
    if(wifi_interface)
    {
        memset(wifi_interface, 0, sizeof(s_ezlopi_wifi_ap_itf_t));
    /*WiFi init*/
        TUYA_CALL_ERR_LOG(tal_wifi_init(wifi_event_callback));
    }
}

VOID ezlopi_wifi_connect_from_id_bin(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    CHAR_T wifi_ssid[]     = "eSpark Wifi";
    CHAR_T wifi_password[] = "47356880";
    if ((NULL != wifi_ssid) && ('\0' != wifi_ssid[0]) &&
        (NULL != wifi_password) && ('\0' != wifi_password[0]))
    {
        // TUYA_CALL_ERR_LOG(ezlopi_wifi_connect(wifi_ssid, wifi_password));
        ezlopi_wifi_connect(wifi_ssid, wifi_password);
    }
}

VOID ezlopi_wifi_connect(const char* ssid, const char* pass)
{
    OPERATE_RET rt = OPRT_OK;
    /*Set WiFi mode to station*/
    TUYA_CALL_ERR_LOG(tal_wifi_set_work_mode(WWM_STATION));
    
    /*STA mode, connect to WiFi*/
    TAL_PR_NOTICE("\r\nconnect wifi ssid: %s, password: %s\r\n", ssid, pass);
    TUYA_CALL_ERR_LOG(tal_wifi_station_connect((SCHAR_T *)ssid, (SCHAR_T *)pass));
}

bool ezlopi_wifi_is_connected()
{
    return is_connected;
}