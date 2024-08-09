#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "ezlopi_cloud_registration.h"
#include "ezlopi_util_trace.h"
// #include "web_provisioning.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_websocket.h"
#include "ezlopi_core_factory_info.h"

static volatile uint32_t is_registered = 0;
static void registration_process(void *pv);

void registration_init(void)
{
    is_registered = 0;
    xTaskCreate(registration_process, "registration_process", 2 * 2048, NULL, 2, NULL);
}
void register_repeat(cJSON* cj_request, cJSON* cj_response)
{
    registration_init();
}

void registered(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_method_str, cJSON_GetObjectItem(cj_request, ezlopi_method_str));
    TRACE_S("Device registration successful.");
    is_registered = 1;
}

static void registration_process(void *pv)
{
    TRACE_D("Registration Process Started!");
    char reg_str[300] = "";
    int register_retry = 0;

    // uint8_t mac_addr[6];
    // char mac_str[18];
    // snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
    //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    char* mac_str = ezlopi_factory_info_v3_get_ezlopi_mac();

    snprintf(reg_str, sizeof(reg_str),
             "{\"id\":\"%u\",\"method\":\"register\",\"params\":"
             "{\"firmware\":\"%s\",\"timeOffset\":18000, \"media\":\"radio\","
             "\"hubType\":\"32.1\",\"mac_address\":\"%s\",\"maxFrameSize\":2048}}",
             rand(), "0.0.0", mac_str);

    cJSON *cjson_data = cJSON_Parse(reg_str);

    while (false == ezlopi_websocket_client_is_connected())
    {
        TRACE_D("Websocket is Disconnected!");
        vTaskDelay(200 / portTICK_RATE_MS);
    }

    while (0 == is_registered)
    {
        TRACE_D("Trying to Register device!");
        char *cjson_str_data = cJSON_Print(cjson_data);
        ezlopi_websocket_client_send(cjson_str_data, strlen(cjson_str_data));
        free(cjson_str_data);
        vTaskDelay(5000 / portTICK_RATE_MS);
        register_retry++;
        if(register_retry >= 5)
        {
            TRACE_E("Unable to Register again!");
            break;
        }

    }

    if (cjson_data)
    {
        cJSON_Delete(cjson_data);
    }
    TRACE_D("Register Task Ended");

    vTaskDelete(NULL);
}
