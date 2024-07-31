#ifndef _BK_WS_CLIENT_H_
#define _BK_WS_CLIENT_H_

#include "esp_err.h"

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

typedef struct esp_websocket_client* esp_websocket_client_handle_t;

typedef enum {
    WEBSOCKET_EVENT_ERROR = 0,       /*!< This event occurs when there are any errors during execution */
    WEBSOCKET_EVENT_ON_CONNECTED,    /*!< Once the HTTP has been connected to the server, no data exchange has been performed */
    WEBSOCKET_EVENT_HEADERS_SENT,     /*!< After sending all the headers to the server */
    WEBSOCKET_EVENT_HEADER_SENT = WEBSOCKET_EVENT_HEADERS_SENT, /*!< This header has been kept for backward compatability
                                                           and will be deprecated in future versions esp-idf */
    WEBSOCKET_EVENT_ON_HEADER,       /*!< Occurs when receiving each header sent from the server */
    WEBSOCKET_EVENT_ON_DATA,         /*!< Occurs when receiving data from the server, possibly multiple portions of the packet */
    WEBSOCKET_EVENT_ON_FINISH,       /*!< Occurs when finish a HTTP session */
    WEBSOCKET_EVENT_DISCONNECTED,    /*!< The connection has been disconnected */
    WEBSOCKET_EVENT_REDIRECT,        /*!< Intercepting HTTP redirects to handle them manually */
} esp_websocket_client_event_id_t;


typedef struct esp_websocket_client_event {
    esp_websocket_client_event_id_t event_id;    /*!< event_id, to know the cause of the event */
    esp_websocket_client_handle_t client;        /*!< esp_http_client_handle_t context */
    void *data;                             /*!< data of the event */
    int data_len;                           /*!< data length of data */
    void *user_data;                        /*!< user_data context, from esp_http_client_config_t user_data */
    char *header_key;                       /*!< For HTTP_EVENT_ON_HEADER event_id, it's store current http header key */
    char *header_value;                     /*!< For HTTP_EVENT_ON_HEADER event_id, it's store current http header value */
    void (*connection_upcall)(bool connected);
    void (*msg_upcall)(const char*, uint32_t)
} esp_websocket_client_event_t;


typedef esp_err_t (*ws_event_handle_cb)(esp_websocket_client_event_t *evt);

typedef struct {
    const char                  *url;                       /*!< Websocket URI, the information on the URI can be overrides the other fields below, if any */
    const char                  *host;                      /*!< Domain or IP as string */
    const char                  *path;                      /*!< HTTP Path, if not set, default is `/` */
    int                         port;                       /*!< Port to connect, default depend on esp_websocket_transport_t (80 or 443) */
    int                         task_prio;                  /*!< Websocket task priority */
    int                         task_stack;                 /*!< Websocket task stack */
    const char                  *cert_pem;                  /*!< Pointer to certificate data in PEM or DER format for server verify (with SSL), default is NULL, not required to verify the server. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in cert_len. */
    size_t                      cert_len;                   /*!< Length of the buffer pointed to by cert_pem. May be 0 for null-terminated pem */
    const char                  *client_cert;               /*!< Pointer to certificate data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_key` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_cert_len. */
    size_t                      client_cert_len;            /*!< Length of the buffer pointed to by client_cert. May be 0 for null-terminated pem */
    const char                  *client_key;                /*!< Pointer to private key data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_key_len */
    size_t                      client_key_len;             /*!< Length of the buffer pointed to by client_key_pem. May be 0 for null-terminated pem */;                /*!< Pointer to private key data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_key_len */                 /*!< SSL Certification, PEM format as string, if the client requires to verify server */
    ws_event_handle_cb          event_handler;
    void (*connection_upcall)(bool connected);
    void (*msg_upcall)(const char*, uint32_t)
}esp_websocket_client_config_t;

struct esp_websocket_client_handle_t {
    esp_websocket_client_config_t config;
};

esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config);
int esp_websocket_client_start(esp_websocket_client_handle_t client);
bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client);
esp_err_t esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char* data, int len, int delay);
#endif /* _BK_WS_CLIENT_H_ */