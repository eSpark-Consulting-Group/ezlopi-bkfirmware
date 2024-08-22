#include "stdint.h"
#include "tal_wifi.h"
// ------------------------------- WiFi related defs ------------------------------- //
typedef AP_IF_S s_ezlopi_wifi_ap_itf_t;
typedef WF_STATION_STAT_E e_ezlopi_wifi_sta_status_t;
typedef WF_IF_E e_ezlopi_wifi_function_type_t;
typedef WF_WK_MD_E e_ezlopi_wifi_working_mode_t;
typedef WF_AP_AUTH_MODE_E e_ezlopi_wifi_auth_mode_t;


// typedef struct ll_ezlopi_wifi_event_upcall
// {
//     void* arg;
//     f_ezlopi_wifi_event_upcall upcall;
//     struct ll_ezlopi_wifi_event_upcall* next;
// } ll_ezlopi_wifi_event_upcall_t;

// typedef struct ezlopi_wifi_status
// {
//     bool wifi_connection;
//     esp_netif_ip_info_t* ip_info;
//     wifi_mode_t wifi_mode;
// } ezlopi_wifi_status_t;

// int ezlopi_wifi_got_ip(void);
// void ezlopi_wifi_set_new_wifi_flag(void);
VOID ezlopi_wifi_initialize(VOID);
VOID ezlopi_wifi_connect(const char* ssid, const char* pass);
VOID ezlopi_wifi_connect_from_id_bin(VOID);
int ezlopi_wait_for_wifi_to_connect(uint32_t wait_time_ms);
NW_IP_S* ezlopi_wifi_get_ip_infos(void);
void ezlopi_wifi_read_mac(uint8_t* mac_addr, WF_IF_E mode);
// esp_netif_ip_info_t* ezlopi_wifi_get_ip_infos(void);
// const char* ezlopi_wifi_get_last_disconnect_reason(void);

// ezlopi_wifi_status_t* ezlopi_wifi_status(void);

// void ezlopi_wifi_event_add(f_ezlopi_wifi_event_upcall upcall, void* arg);

// int ezlopi_wifi_get_wifi_mac(uint8_t mac[6]);

OPERATE_RET ezlopi_wifi_scan_start(CHAR_T ***ssids, CHAR_T ***rssis, UINT_T *num_aps);
void ezlopi_wifi_scan_stop();
bool ezlopi_wifi_is_connected();