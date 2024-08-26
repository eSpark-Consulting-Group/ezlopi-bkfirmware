#include "tuya_cloud_types.h"
#include "tuya_iot_wifi_api.h"
#include "ezlopi_core_wifi.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_event_group.h"
// #include "base_event.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

bool is_connected = false;
static NW_IP_S sta_info;

static int sg_retry_num = 0;
static volatile int sg_station_got_ip = 0;
static const char* const scg_wifi_no_error_str = "NO_ERROR";
static const char* sg_last_disconnect_reason = "NO_ERROR";

/* Prototypes */
static void __event_wifi_disconnected(void* event_data);


STATIC VOID_T wifi_event_callback(WF_EVENT_E event, VOID_T *arg)
{
    OPERATE_RET op_ret = OPRT_OK;
    memset(&sta_info, 0, SIZEOF(NW_IP_S));

    TRACE_D("-------------event callback-------------");
    switch (event) {
        case WFE_CONNECTED:{
            is_connected = true;
            TRACE_D("connection succeeded!");

            /* output ip information */
            op_ret = tal_wifi_get_ip(WF_STATION, &sta_info);
            if (OPRT_OK != op_ret) {
                TRACE_E("get station ip error");
                return;
            }

#ifdef nwipstr                
            if(IS_NW_IPV4_ADDR(&sta_info)) {
                TRACE_I("gw: %s", sta_info.nwgwstr);
                TRACE_I("ip: %s", sta_info.nwipstr);
                TRACE_I("mask: %s", sta_info.nwmaskstr);
            }else {
                TRACE_I("ip: %s", sta_info.addr.ip6.ip);
                TRACE_I("islinklocal: %d", sta_info.addr.ip6.islinklocal);
            }
#else 
            TRACE_I("gw: %s", sta_info.gw);
            TRACE_I("ip: %s", sta_info.ip);
            TRACE_I("mask: %s", sta_info.mask);
#endif
            ezlopi_event_group_set_event(EZLOPI_EVENT_WIFI_CONNECTED);
            break;
        }

        case WFE_CONNECT_FAILED: {
            TRACE_D("connection fail!");
            is_connected = false;
            __event_wifi_disconnected(arg);
            break;
        }

        case WFE_DISCONNECTED: {
            TRACE_D("disconnect!");
            is_connected = false;
            __event_wifi_disconnected(arg);
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
    TRACE_I("WIFI INIT");
    OPERATE_RET rt = OPRT_OK;
    s_ezlopi_wifi_ap_itf_t *wifi_interface = (s_ezlopi_wifi_ap_itf_t *)malloc(sizeof(s_ezlopi_wifi_ap_itf_t));
    if(wifi_interface)
    {
        memset(wifi_interface, 0, sizeof(s_ezlopi_wifi_ap_itf_t));
    /*WiFi init*/
        // tuya_iot_wf_timeout_set(60);
        BK_CALL_ERR_LOG(tal_wifi_init(wifi_event_callback));
    }
    ezlopi_wifi_connect_from_id_bin();
}

VOID ezlopi_wifi_connect_from_id_bin(VOID)
{
    OPERATE_RET rt = OPRT_OK;
    // CHAR_T *wifi_ssid     = ezlopi_factory_info_v3_get_ssid();
    // CHAR_T *wifi_password = ezlopi_factory_info_v3_get_password();

    CHAR_T *wifi_ssid     = "Embedded";
    CHAR_T *wifi_password = "123asd456asd";

    if ((NULL != wifi_ssid) && ('\0' != wifi_ssid[0]) &&
        (NULL != wifi_password) && ('\0' != wifi_password[0]))
    {
        // TBK_CALL_ERR_LOG(ezlopi_wifi_connect(wifi_ssid, wifi_password));
        ezlopi_wifi_connect(wifi_ssid, wifi_password);
    }
    vTaskDelay(500/portTICK_PERIOD_MS);
}

VOID ezlopi_wifi_connect(const char* ssid, const char* pass)
{
    OPERATE_RET rt = OPRT_OK;
    /*Set WiFi mode to station*/
    BK_CALL_ERR_LOG(tal_wifi_set_work_mode(WWM_STATION));
    
    /*STA mode, connect to WiFi*/
    TRACE_I("\r\nconnect wifi ssid: %s, password: %s\r\n", ssid, pass);
    // tal_wifi_lp_enable();
    // BK_CALL_ERR_LOG(tal_wifi_station_connect((SCHAR_T *)ssid, (SCHAR_T *)pass));
    BK_CALL_ERR_LOG(tkl_wifi_station_connect((SCHAR_T *)ssid, (SCHAR_T *)pass));
}

bool ezlopi_wifi_is_connected()
{
    return is_connected;
}

NW_IP_S* ezlopi_wifi_get_ip_infos()
{
    return &sta_info;
}

void ezlopi_wifi_read_mac(uint8_t* mac_addr, WF_IF_E mode)
{
    tal_wifi_get_mac(mode, mac_addr);
}

#if 0
void ezlopi_wifi_scan_start()
{
    AP_IF_S *ap_list = NULL;
    UINT_T num_aps = 0;
    OPERATE_RET result = tkl_wifi_scan_ap(NULL, &ap_list, &num_aps);

    if (result == OPRT_OK) {
        TRACE_S("Found %u APs:\n", num_aps);
        for (UINT_T i = 0; i < num_aps; ++i) {
            TRACE_I("AP %u: SSID: %s, Signal Strength: %d\n", i + 1, ap_list[i].ssid, ap_list[i].rssi);
            // You might have other fields in AP_IF_S to print or use.
        }
    } else {
        TRACE_E("Wi-Fi scan failed or function not supported. Error code: %d\n", result);
    }

    // Don't forget to free the memory if necessary
    if (ap_list) {
        free(ap_list);
    }

    return 0;
}
#endif


OPERATE_RET ezlopi_wifi_scan_start(CHAR_T ***ssids, CHAR_T ***rssis, UINT_T *num_aps)
{
    AP_IF_S *ap_list = NULL;
    *num_aps = 0;
    OPERATE_RET result = tkl_wifi_scan_ap(NULL, &ap_list, num_aps);

    if (result == OPRT_OK && *num_aps > 0) {
        // Allocate memory for SSID and RSSI arrays
        *ssids = (CHAR_T**)malloc(*num_aps * sizeof(CHAR_T*));
        *rssis = (CHAR_T**)malloc(*num_aps * sizeof(CHAR_T*));

        if (!(*ssids) || !(*rssis)) {
            TRACE_E("Memory allocation failed\n");
            free(ap_list);
            if (*ssids) free(*ssids);
            if (*rssis) free(*rssis);
            return OPRT_COM_ERROR; // Return a suitable error code
        }

        // Populate the SSID and RSSI arrays
        for (UINT_T i = 0; i < *num_aps; ++i) {
            (*ssids)[i] = strdup(ap_list[i].ssid); // Allocate and copy SSID
            (*rssis)[i] = (CHAR_T*)malloc(10);     // Allocate space for RSSI string
            snprintf((*rssis)[i], 10, "%d", ap_list[i].rssi); // Convert RSSI to string
        }

        // TRACE_S("Found %u APs. Arrays populated.\n", *num_aps);
    } else {
        TRACE_E("Wi-Fi scan failed or function not supported. Error code: %d\n", result);
        return result;
    }

    // Free the memory allocated for AP list
    if (ap_list) {
        free(ap_list);
    }

    return OPRT_OK;
}

void ezlopi_wifi_scan_stop()
{

}


static void __event_wifi_disconnected(void* event_data)
{
    TRACE_E("%s, Enter", __FUNCTION__);
    ezlopi_event_group_clear_event(EZLOPI_EVENT_WIFI_CONNECTED);

    // if (event_data)
    {
        // event_data; //
        // WF_DISCONN_REASON_E* disconnected_reason = (WF_DISCONN_REASON_E*)event_data;
        // TRACE_E("Disconnect reason[%d]: %s", disconnected->reason, ezlopi_wifi_err_reason_str(disconnected->reason));
        TRACE_E("Disconnect reason[%d]: %s", 1, "Disconnect Reason");
        // sg_last_disconnect_reason = ezlopi_wifi_err_reason_str(disconnected->reason);
        sg_last_disconnect_reason = "Disconnect Reason";

        sg_station_got_ip = 0;
        // esp_wifi_connect();
        ezlopi_wifi_connect_from_id_bin();

        if (sg_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            sg_retry_num++;
            TRACE_S("retry to connect to the AP");
        }
        else
        {
            ezlopi_event_group_set_event(EZLOPI_EVENT_WIFI_FAIL);
            sg_retry_num = 0;
        }

        TRACE_W("connect to the AP fail");
        memset(&sta_info, 0, sizeof(NW_IP_S));
    }
}

int ezlopi_wait_for_wifi_to_connect(uint32_t wait_time_ms)
{
    uint32_t ret = 0;
    while (-1 == ezlopi_event_group_wait_for_event(EZLOPI_EVENT_WIFI_CONNECTED, wait_time_ms, false))
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ret = ezlopi_event_group_wait_for_event(EZLOPI_EVENT_WIFI_CONNECTED, wait_time_ms, false);
    return ret;
}