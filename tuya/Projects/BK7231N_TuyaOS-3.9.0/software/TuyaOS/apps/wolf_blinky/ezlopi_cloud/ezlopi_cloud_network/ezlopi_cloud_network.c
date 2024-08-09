#include <string.h>
// #include "esp_wifi.h"
// #include "esp_netif_types.h"
#include "ezlopi_cloud_network.h"
#include "ezlopi_core_wifi.h"
// #include "ezlopi_core_ethernet.h"
#include "ezlopi_util_trace.h"

#include "cJSON.h"
#include "ezlopi_cloud_methods_str.h"
#include "ezlopi_cloud_keywords.h"

#define MAC_ADDR_EXPANDED(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]

void network_get(cJSON* cj_request, cJSON* cj_response)
{
    cJSON* cjson_result = cJSON_AddObjectToObject( cj_response, ezlopi_result_str);
    if (cjson_result)
    {
        cJSON* interfaces_array = cJSON_AddArrayToObject( cjson_result, ezlopi_interfaces_str);
        if (interfaces_array)
        {
            cJSON* wifi_properties = cJSON_CreateObject();
            if (wifi_properties)
            {
                char tmp_string[64];
                cJSON_AddStringToObject( wifi_properties, ezlopi__id_str, ezlopi_wifi_str);
                cJSON_AddStringToObject( wifi_properties, ezlopi_enabled_str, ezlopi_auto_str);

                uint8_t mac_addr[6] = {0};
                ezlopi_wifi_read_mac(mac_addr, WF_STATION);
                snprintf(tmp_string, sizeof(tmp_string), "%02x:%02x:%02x:%02x:%02x:%02x", MAC_ADDR_EXPANDED(mac_addr));
                cJSON_AddStringToObject( wifi_properties, ezlopi_hwaddr_str, tmp_string);
                cJSON_AddBoolToObject( wifi_properties, ezlopi_internetAvailable_str, true);
                cJSON_AddStringToObject( wifi_properties, ezlopi_network_str, ezlopi_wan_str);
                cJSON_AddStringToObject( wifi_properties, ezlopi_status_str, ezlopi_up_str);
                cJSON_AddStringToObject( wifi_properties, ezlopi_type_str, ezlopi_wifi_str);

                cJSON* wifi_ipv4 = cJSON_CreateObject();
                if (wifi_ipv4)
                {
                    NW_IP_S* ip_info = ezlopi_wifi_get_ip_infos();
                    snprintf(tmp_string, sizeof(tmp_string), ip_info->nwgwstr);
                    cJSON_AddStringToObject( wifi_ipv4, ezlopi_ip_str, tmp_string);
                    snprintf(tmp_string, sizeof(tmp_string), ip_info->nwipstr);
                    cJSON_AddStringToObject( wifi_ipv4, ezlopi_mask_str, tmp_string);
                    snprintf(tmp_string, sizeof(tmp_string), ip_info->nwmaskstr);
                    cJSON_AddStringToObject( wifi_ipv4, ezlopi_gateway_str, tmp_string);
                    cJSON_AddStringToObject( wifi_ipv4, ezlopi_mode_str, ezlopi_dhcp_str);

                    if (!cJSON_AddItemToObjectCS( wifi_properties, ezlopi_ipv4_str, wifi_ipv4))
                    {
                        cJSON_Delete( wifi_ipv4);
                        wifi_ipv4 = NULL;
                    }
                }

                if (!cJSON_AddItemToArray(interfaces_array, wifi_properties))
                {
                    cJSON_Delete( wifi_properties);
                }
            }
        }
    }
}

#if 0
static int get_wifi_properties(cJSON* interface_array)
{
    int ret = 0;

    cJSON* wifi_properties = cJSON_CreateObject();
    if (wifi_properties)
    {
        char tmp_string[64];
        cJSON_AddStringToObject( wifi_properties, ezlopi__id_str, ezlopi_wifi_str);
        cJSON_AddStringToObject( wifi_properties, ezlopi_enabled_str, ezlopi_auto_str);

        cJSON* wifi_ipv4 = cJSON_CreateObject();
        if (wifi_ipv4)
        {
            esp_netif_ip_info_t* ip_info = ezlopi_wifi_get_ip_infos();
            snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->gw));
            cJSON_AddStringToObject( wifi_ipv4, ezlopi_ip_str, tmp_string);
            snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->ip));
            cJSON_AddStringToObject( wifi_ipv4, ezlopi_mask_str, tmp_string);
            snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->netmask));
            cJSON_AddStringToObject( wifi_ipv4, ezlopi_gateway_str, tmp_string);
            cJSON_AddStringToObject( wifi_ipv4, ezlopi_mode_str, ezlopi_dhcp_str);

            cJSON* wifi_ipv4 = cJSON_CreateObject();
            if (wifi_ipv4)
            {
                esp_netif_ip_info_t* ip_info = ezlopi_wifi_get_ip_infos();
                snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->gw));
                cJSON_AddStringToObject( wifi_ipv4, ezlopi_gateway_str, tmp_string);
                snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->ip));
                cJSON_AddStringToObject( wifi_ipv4, ezlopi_ip_str, tmp_string);
                snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->netmask));
                cJSON_AddStringToObject( wifi_ipv4, ezlopi_mask_str, tmp_string);
                cJSON_AddStringToObject( wifi_ipv4, ezlopi_mode_str, ezlopi_dhcp_str);

                if (!cJSON_AddItemToObjectCS( wifi_properties, ezlopi_ipv4_str, wifi_ipv4))
                {
                    cJSON_Delete( wifi_ipv4);
                    wifi_ipv4 = NULL;
                }
            }
            cJSON_AddStringToObject( wifi_properties, ezlopi_network_str, ezlopi_wan_str);
            cJSON_AddStringToObject( wifi_properties, ezlopi_status_str, ezlopi_up_str);
            cJSON_AddStringToObject( wifi_properties, ezlopi_type_str, ezlopi_wifi_str);

            if (!cJSON_AddItemToArray(interface_array, wifi_properties))
            {
                cJSON_Delete( wifi_properties);
            }
        }
        else
        {
            ret = 1;
        }
        return ret;
    }
}
#endif

#if 0
void network_wifi_scan_start(cJSON* cj_request, cJSON* cj_response)
{
    CHAR_T **ssids = NULL;
    CHAR_T **rssis = NULL;
    UINT_T num_aps = 0;
    OPERATE_RET result = ezlopi_wifi_scan_start(&ssids, &rssis, &num_aps);

    if (result == OPRT_OK) {
        for (UINT_T i = 0; i < num_aps; ++i) {
            TRACE_S("AP %u: SSID: %s, RSSI: %s", i + 1, ssids[i], rssis[i]);
            free(ssids[i]); // Free each SSID string
            free(rssis[i]); // Free each RSSI string
        }
        free(ssids); // Free the array of SSID strings
        free(rssis); // Free the array of RSSI strings
    } else {
        TRACE_E("Wi-Fi scan failed or function not supported. Error code: %d\n", result);
    }
    cJSON_AddObjectToObject( cj_response, ezlopi_result_str);
}
#endif

void network_wifi_scan_start(cJSON* cj_request, cJSON* cj_response)
{
    CHAR_T **ssids = NULL;
    CHAR_T **rssis = NULL;
    UINT_T num_aps = 0;
    OPERATE_RET result = ezlopi_wifi_scan_start(&ssids, &rssis, &num_aps);

    if (result == OPRT_OK) {
        // Create a JSON array to hold the AP information
        cJSON *ap_array = cJSON_CreateArray();

        for (UINT_T i = 0; i < num_aps; ++i) {
            int rssi_value = atoi(rssis[i]); // Convert RSSI string to integer

            // Only add APs with RSSI greater than -60
            if (rssi_value > -60) {
                // Create a JSON object for each AP
                cJSON *ap_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(ap_obj, "SSID", ssids[i]);
                cJSON_AddStringToObject(ap_obj, "RSSI", rssis[i]);

                // Add the AP object to the array
                cJSON_AddItemToArray(ap_array, ap_obj);
            }

            // Free the memory for SSID and RSSI strings
            free(ssids[i]);
            free(rssis[i]);
        }

        // Add the AP array to the response object if it's not empty
        if (cJSON_GetArraySize(ap_array) > 0) {
            cJSON_AddItemToObject(cj_response, ezlopi_result_str, ap_array);
        } else {
            cJSON_AddStringToObject(cj_response, "message", "No APs found with RSSI greater than -60.");
            cJSON_Delete(ap_array); // Free the array if not used
        }

        // Free the arrays themselves
        free(ssids);
        free(rssis);
    } else {
        TRACE_E("Wi-Fi scan failed or function not supported. Error code: %d\n", result);
        cJSON_AddStringToObject(cj_response, "error", "Wi-Fi scan failed or function not supported.");
    }
}


void network_wifi_scan_stop(cJSON* cj_request, cJSON* cj_response)
{
    ezlopi_wifi_scan_stop();
    cJSON_AddObjectToObject( cj_response, ezlopi_result_str);
}

