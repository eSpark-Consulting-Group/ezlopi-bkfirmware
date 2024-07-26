#ifndef ESP_HTTP_CLIENT_H
#define ESP_HTTP_CLIENT_H

#include "esp_err.h"
#include "esp_http_client.h"
#include "stddef.h"
#include "stdbool.h"
struct esp_http_client_event_t;
typedef struct esp_http_client *esp_http_client_handle_t;

typedef enum {
    HTTP_EVENT_ERROR = 0,       /*!< This event occurs when there are any errors during execution */
    HTTP_EVENT_ON_CONNECTED,    /*!< Once the HTTP has been connected to the server, no data exchange has been performed */
    HTTP_EVENT_HEADERS_SENT,     /*!< After sending all the headers to the server */
    HTTP_EVENT_HEADER_SENT = HTTP_EVENT_HEADERS_SENT, /*!< This header has been kept for backward compatability
                                                           and will be deprecated in future versions esp-idf */
    HTTP_EVENT_ON_HEADER,       /*!< Occurs when receiving each header sent from the server */
    HTTP_EVENT_ON_DATA,         /*!< Occurs when receiving data from the server, possibly multiple portions of the packet */
    HTTP_EVENT_ON_FINISH,       /*!< Occurs when finish a HTTP session */
    HTTP_EVENT_DISCONNECTED,    /*!< The connection has been disconnected */
    HTTP_EVENT_REDIRECT,        /*!< Intercepting HTTP redirects to handle them manually */
} esp_http_client_event_id_t;


typedef struct esp_http_client_event {
    esp_http_client_event_id_t event_id;    /*!< event_id, to know the cause of the event */
    esp_http_client_handle_t client;        /*!< esp_http_client_handle_t context */
    void *data;                             /*!< data of the event */
    int data_len;                           /*!< data length of data */
    void *user_data;                        /*!< user_data context, from esp_http_client_config_t user_data */
    char *header_key;                       /*!< For HTTP_EVENT_ON_HEADER event_id, it's store current http header key */
    char *header_value;                     /*!< For HTTP_EVENT_ON_HEADER event_id, it's store current http header value */
} esp_http_client_event_t;

/**
 * @brief HTTP method
 */
typedef enum {
    HTTP_METHOD_GET = 0,    /*!< HTTP GET Method */
    HTTP_METHOD_POST,       /*!< HTTP POST Method */
    HTTP_METHOD_PUT,        /*!< HTTP PUT Method */
    HTTP_METHOD_PATCH,      /*!< HTTP PATCH Method */
    HTTP_METHOD_DELETE,     /*!< HTTP DELETE Method */
    HTTP_METHOD_HEAD,       /*!< HTTP HEAD Method */
    HTTP_METHOD_NOTIFY,     /*!< HTTP NOTIFY Method */
    HTTP_METHOD_SUBSCRIBE,  /*!< HTTP SUBSCRIBE Method */
    HTTP_METHOD_UNSUBSCRIBE,/*!< HTTP UNSUBSCRIBE Method */
    HTTP_METHOD_OPTIONS,    /*!< HTTP OPTIONS Method */
    HTTP_METHOD_COPY,       /*!< HTTP COPY Method */
    HTTP_METHOD_MOVE,       /*!< HTTP MOVE Method */
    HTTP_METHOD_LOCK,       /*!< HTTP LOCK Method */
    HTTP_METHOD_UNLOCK,     /*!< HTTP UNLOCK Method */
    HTTP_METHOD_PROPFIND,   /*!< HTTP PROPFIND Method */
    HTTP_METHOD_PROPPATCH,  /*!< HTTP PROPPATCH Method */
    HTTP_METHOD_MKCOL,      /*!< HTTP MKCOL Method */
    HTTP_METHOD_MAX,
} esp_http_client_method_t;

typedef esp_err_t (*http_event_handle_cb)(esp_http_client_event_t *evt);
typedef struct {
    const char* url;
    const char* host;
    const char* path;    
    const char* user_agent;
    const char* cert_pem;
    size_t cert_pem_len;
    const char* client_cert_pem;
    size_t client_cert_pem_len;
    const char* client_key_pem;
    size_t client_key_pem_len;
    esp_http_client_method_t    method;
    http_event_handle_cb        event_handler;
    void* user_data;
    int port;
}esp_http_client_config_t;

struct esp_http_client_handle_t {
    esp_http_client_config_t config;
};

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_perform(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
esp_err_t esp_http_client_set_method(esp_http_client_handle_t client, esp_http_client_method_t method);
#endif //ESP_HTTP_CLIENT_H