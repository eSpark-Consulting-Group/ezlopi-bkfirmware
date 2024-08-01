#include "ezlopi_util_trace.h"
#include "FreeRTOS.h"
#include "ezlopi_core_websocket.h"
#include "cert.h"
#include "cJSON.h"

// Web Socket Configurations
#define EZPI_CORE_WSS_TASK_STACK_SIZE 4096 // 4 * 1024

static esp_websocket_client_handle_t client = NULL;
static void websocket_event_handler(esp_websocket_client_event_t *evt);

int ezlopi_websocket_client_send(char *data, uint32_t len)
{
    int ret = 0;

    if ((NULL != data) && (len > 0))
    {
        if (esp_websocket_client_is_connected(client) && (len > 0) && (NULL != data))
        {
            ret = esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
        }
    }

    return ret;
}

esp_websocket_client_handle_t ezlopi_websocket_client_init(cJSON* uri, void (*msg_upcall)(const char*, uint32_t), void (*connection_upcall)(bool connected))
{
    if ((NULL == client) && (NULL != uri) && (NULL != uri->valuestring)  && (NULL != msg_upcall)
     )
    {
        char * ca_cert = CA_CERT;
        char * ssl_priv = SSL_PRIVATE_KEY;
        char * ssl_shared = SHARED_KEY;

        esp_websocket_client_config_t websocket_cfg = {
            .url = uri->valuestring,
            .task_stack = EZPI_CORE_WSS_TASK_STACK_SIZE,
            .cert_pem = ca_cert,
            .client_cert = ssl_shared,
            .client_key = ssl_priv,
            .event_handler = websocket_event_handler,
            .connection_upcall = connection_upcall,
            .msg_upcall = msg_upcall,
            .port = 443,
        };

        char websocket_url_buffer[128] = {0};

        TRACE_D("Connecting to %s...", websocket_cfg.url);
        strncpy(websocket_url_buffer, websocket_cfg.url, sizeof(websocket_url_buffer) - 1);
        websocket_url_buffer[sizeof(websocket_url_buffer) - 1] = '\0';        
        websocket_cfg.url = websocket_url_buffer;

        client = esp_websocket_client_init(&websocket_cfg);
        if(client)
        {
            esp_websocket_client_start(client);
        }

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