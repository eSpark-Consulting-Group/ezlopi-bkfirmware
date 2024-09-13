#include "FreeRTOS.h"
#include "tal_thread.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_websocket.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_event_group.h"
#include "cJSON.h"

// Web Socket Configurations
#define EZPI_CORE_WSS_TASK_STACK_SIZE 4096 // 4 * 1024

static esp_websocket_client_handle_t client = NULL;
static void websocket_event_handler(esp_websocket_client_event_t *evt);

int ezlopi_websocket_client_send(char *data, uint32_t len)
{
    int ret = ESP_FAIL;

    if ((NULL != data) && (len > 0))
    {
        if (esp_websocket_client_is_connected(client) && (len > 0) && (NULL != data))
        {
            ret = esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
        }
    }

    return ret;
}
static void (*pv_msg_upcall)(const char*, uint32_t);
static void (*pv_connection_upcall)(bool connected);

void ezlopi_websocket_register_upcalls(void (*msg_upcall)(const char*, uint32_t), void (*connection_upcall)(bool connected))
{
    pv_msg_upcall = msg_upcall;
    pv_connection_upcall = connection_upcall;
}

esp_websocket_client_handle_t ezlopi_websocket_client_init(cJSON* uri)
{
    TRACE_D("In Function %s", __FUNCTION__);
    if ((NULL == client) && (NULL != uri) && (NULL != uri->valuestring)  && (NULL != pv_msg_upcall)
     )
    {
        char * ca_cert = ezlopi_factory_info_v3_get_ca_certificate();
        char * ssl_priv = ezlopi_factory_info_v3_get_ssl_private_key();
        char * ssl_shared = ezlopi_factory_info_v3_get_ssl_shared_key();

        esp_websocket_client_config_t websocket_cfg = {
            .url = uri->valuestring,
            .task_stack = EZPI_CORE_WSS_TASK_STACK_SIZE,
            .cert_pem = ca_cert,
            .client_cert = ssl_shared,
            .client_key = ssl_priv,
            .event_handler = websocket_event_handler,
            .connection_upcall = pv_connection_upcall,
            .msg_upcall = pv_msg_upcall,
            .port = 443,
        };

        char websocket_url_buffer[128] = {0};

        TRACE_I("Connecting to %s...", websocket_cfg.url);
        strncpy(websocket_url_buffer, websocket_cfg.url, sizeof(websocket_url_buffer) - 1);
        websocket_cfg.url = websocket_url_buffer;

        client = esp_websocket_client_init(&websocket_cfg);
        if(client)
        {
            esp_websocket_client_start(client);
        }
        ezlopi_event_group_set_event(EZLOPI_EVENT_WS_CONNECT);

    }
    else
    {
        TRACE_E("%s invalid if", __FUNCTION__);
    }
}

static void websocket_event_handler(esp_websocket_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case WEBSOCKET_EVENT_ON_CONNECTED:
    {
        TRACE_D("WEBSOCKET_EVENT_CONNECTED");
        if(evt && evt->connection_upcall)
        {
            evt->connection_upcall(true);
        }
        break;
    }
    case WEBSOCKET_EVENT_DISCONNECTED:
    {
        TRACE_D("WEBSOCKET_EVENT_DISCONNECTED");
        if(evt && evt->connection_upcall)
        {
            evt->connection_upcall(false);
        }
        break;
    }
    case WEBSOCKET_EVENT_ON_DATA:
    {
        TRACE_D("WEBSOCKET_EVENT_ON_DATA");
        if(evt && evt->data && evt->data_len && evt->msg_upcall)
        {
            evt->msg_upcall(evt->data, evt->data_len);
        }
        break;
    }
    case WEBSOCKET_EVENT_ERROR:
    {
        TRACE_D("WEBSOCKET_EVENT_ERROR");
        break;
    }
    case WEBSOCKET_EVENT_RECONNECT:
        TRACE_D("WEBSOCKET_EVENT_RECONNECT");
        break;
    default:
    {
        TRACE_D("Websocket event type not-implemented! value: %u", evt->event_id);
        break;
    }
    }
}

bool ezlopi_websocket_client_is_connected()
{
    return esp_websocket_client_is_connected(client);
}

/* Tinkering with websocket reconnect 

THREAD_HANDLE ezlopi_websocket_reconnect_handle = NULL;

void ezlopi_websocket_reconnect_task()
{
    while(client != NULL)
    {
        TRACE_E("Waiting for Client to be NULL");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    cJSON* json_uri = cJSON_CreateObject(); 
    cJSON_AddStringToObject(json_uri, "uri", "wss://nma-server9-cloud.ezlo.com:443"); //TODO: get this wss url from NVS
    cJSON* uri_item = cJSON_GetObjectItemCaseSensitive(json_uri, "uri");
    TRACE_I("Reconnecting %s", uri_item->valuestring);
    ezlopi_websocket_client_init(uri_item);

    tal_thread_delete(ezlopi_websocket_reconnect_handle);
}

void ezlopi_websocket_reconnect()
{
    THREAD_CFG_T cfg = {2*2048, THREAD_PRIO_6, "ezlo-ws-rc"};
    tal_thread_create_and_start(&ezlopi_websocket_reconnect_handle, NULL, NULL, ezlopi_websocket_reconnect_task, NULL, &cfg);
}
*/
