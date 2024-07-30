#include "bk_websocket_client.h"
#include "esp_err.h"
#include "tal_log.h"
#include "tal_network.h"
#include "tal_thread.h"
#include "string.h"

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netdb.h"

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define RX_TX_BUFFER_SIZE 256
#define WS_UPGRADE_REQ "GET / HTTP/1.1\r\nHost: nma-server6-cloud.ezlo.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-Websocket-Extensions: premessage-deflate; client_max_window_bits\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: chat, superchat\r\n\r\n"

THREAD_HANDLE bk_ws_rcv_thread = NULL;

typedef enum {
    WEBSOCKET_STATE_UNINIT = 0,
    WEBSOCKET_STATE_INIT,
    WEBSOCKET_STATE_CONNECTED,
    WEBSOCKET_STATE_REQ_COMPLETE_HEADER,
    WEBSOCKET_STATE_REQ_COMPLETE_DATA,
    WEBSOCKET_STATE_RES_COMPLETE_HEADER,
    WEBSOCKET_STATE_RES_ON_DATA_START,
    WEBSOCKET_STATE_RES_COMPLETE_DATA,
    WEBSOCKET_STATE_CLOSE
} esp_http_state_t;

/**
 * ws Buffer
 */
typedef struct {
    char *data;         /*!< The ws data received from the server */
    int len;            /*!< The ws data len received from the server */
    char *raw_data;     /*!< The ws data after decoding */
    char *orig_raw_data;/*!< The Original pointer to ws data after decoding */
    int raw_len;        /*!< The ws data len after decoding */
    char *output_ptr;   /*!< The destination address of the data to be copied to after decoding */
} esp_ws_buffer_t;

/**
 * private ws Data structure
 */
typedef struct {
    esp_ws_buffer_t   *buffer;        /*!< data buffer as linked list */
    int                 status_code;    /*!< status code (integer) */
    int64_t             content_length; /*!< data length */
    int                 method;         /*!< ws method */
} esp_ws_data_t;

struct esp_websocket_client {
    const char* url;
    const char* host;
    const char* path;    
    const char* port;
    esp_ws_data_t *request;
    esp_ws_data_t *response;
    void* user_data;
    ws_event_handle_cb        event_handler;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    int sockfd;
    int state;
    int method;
    void (*connection_upcall)(bool connected);
    void (*msg_upcall)(const char*, uint32_t)
};

/* Prototypes */
void ws_receive(esp_websocket_client_handle_t client);
esp_err_t parse_url(esp_websocket_client_handle_t client, const char* url);

esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config)
{
    esp_websocket_client_handle_t client;
    esp_err_t ret = ESP_OK;
    bool _success = (
        (client                         = calloc(1, sizeof(esp_websocket_client_handle_t)))           &&
        (client->request                = calloc(1, sizeof(esp_ws_data_t)))             &&
        (client->request->buffer        = calloc(1, sizeof(esp_ws_buffer_t)))           &&
        (client->response               = calloc(1, sizeof(esp_ws_data_t)))             &&
        (client->response->buffer       = calloc(1, sizeof(esp_ws_buffer_t)))
    );
    if (!_success) {
        TAL_PR_DEBUG("Error allocate memory");
        goto error;
    }

    _success = 
    (
                (client->request->buffer->data  = calloc(1, RX_TX_BUFFER_SIZE))  &&
                (client->response->buffer->data = calloc(1, RX_TX_BUFFER_SIZE))
    );

    if (!_success) {
        TAL_PR_DEBUG("Error allocate memory for req res buffers");
        goto error;
    }
    
    if( NULL == (config->url || config->event_handler) )
    {
        TAL_PR_DEBUG("Error url or handler NULL");
        goto error;
    }
    client->url = config->url;
    client->event_handler = config->event_handler;
    client->connection_upcall = config->connection_upcall;

    TAL_PR_DEBUG("URLLLLL %s", config->url);

    ret = parse_url(client, config->url);
    if(ret != ESP_OK)
    {
        TAL_PR_DEBUG("ERROR parse_url error num = %s", ret);
        return NULL;
    }

    if(ws_socket_initialize(config, client)!= ESP_OK)
    {
        TAL_PR_DEBUG("ERROR ws_socket_initialize error num = %s", ret);
        return NULL;
    }

    ws_ssl_initialize(config, client);

    if(ret == ESP_OK)
    {
        return client;
    }

error:
    esp_ws_client_cleanup(client);
    return ESP_ERR;
}

int esp_websocket_client_start(esp_websocket_client_handle_t client)
{
    ws_server_connect(client);
}

void esp_ws_client_cleanup(esp_websocket_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    esp_websocket_client_event_t ws_event;
    /* Bidirectional shutdown */
    while (wolfSSL_shutdown(client->ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        TAL_PR_DEBUG("Shutdown not complete\n");
        ret = ESP_ERR;
        return ret;
    }
    wolfSSL_free(client->ssl);      /* Free the wolfSSL object                  */
    wolfSSL_CTX_free(client->ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment              */
    
    if(client->request)
    {
        if (client->request->buffer) {
            free(client->request->buffer->data);
        }
        free(client->request->buffer);
        free(client->request);
    }
    if (client->response) {
        if (client->response->buffer) {
            free(client->response->buffer->data);
        }
        free(client->response->buffer);
        free(client->response);
    }
    free(client->host);
    free(client->path);
    free(client);

    ws_event.connection_upcall = client->connection_upcall;
    ws_event.event_id = WEBSOCKET_EVENT_DISCONNECTED;
    client->event_handler(&ws_event);

    close(client->sockfd);    
    return ret;
}

int ws_socket_initialize(const esp_websocket_client_config_t *config, esp_websocket_client_handle_t client)
{
    // int sockfd;
    struct addrinfo hints, *result;
    size_t len;
    int ret = ESP_OK;

    wolfSSL_Debugging_ON();
    wolfSSL_SetLoggingCb(tal_log_print);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Create a socket */
    client->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client->sockfd == -1) {
        TAL_PR_ERR("ERROR: failed to create the socket\n");
        return ESP_ERR;
    }

    TAL_PR_DEBUG("SOCKET CREATE SUCCESS!\n");

    /* Resolve domain name to IP address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(client->host, NULL, &hints, &result) != 0) {
        TAL_PR_ERR("ERROR: failed to resolve domain name\n");
        close(client->sockfd);
        return ESP_ERR;
    }

    /* Print the resolved IP address */
    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    addr->sin_port = config->port;
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
    TAL_PR_DEBUG("Resolved IP address: %s\n", ipstr);

    /* Connect to the server */
    if(tal_net_connect(client->sockfd, tal_net_str2addr(ipstr), addr->sin_port) < 0) {
        TAL_PR_ERR("ERROR: failed to connect to server\n");
        close(client->sockfd);
        freeaddrinfo(result);
        return ESP_ERR;
    }


    TAL_PR_DEBUG("SOCKET CONNECT SUCCESS!\n");
    return ret;
}

int ws_ssl_initialize(esp_websocket_client_config_t *config, esp_websocket_client_handle_t client)
{
    int ret = ESP_OK;
    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to initialize the library\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("SSL INIT SUCCESS!\n");

    /* Create and initialize WOLFSSL_CTX */
#ifdef WOLFSSL_TLS13
    client->ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
#else
    client->ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    // client->ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
#endif
    if (client->ctx == NULL) {
        TAL_PR_DEBUG("ERROR: failed to create WOLFSSL_CTX\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("wolfSSL_CTX_new SUCCESS!\n");

    /* Loading keys */
    if (wolfSSL_CTX_use_PrivateKey_buffer(client->ctx, config->client_key, strlen(config->client_key), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG(stderr, "Error loading client private key\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("\t PRIVATE KEY LOADED\n");

    // Load client's certificate
    if (wolfSSL_CTX_use_certificate_buffer(client->ctx, config->client_cert, strlen(config->client_cert), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG(stderr, "Error loading client certificate\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("\t SHARED KEY LOADED\n");

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_buffer(client->ctx, config->cert_pem, strlen(config->cert_pem), WOLFSSL_FILETYPE_PEM))
         != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to load %s, please check the Cert. err: %d\n", config->cert_pem, ret);
        return ESP_ERR;
    }
    
    TAL_PR_DEBUG("wolfSSL_CTX_load_verify_buffer SUCCESS!\n");

    /* Create a WOLFSSL object */
    if ((client->ssl = wolfSSL_new(client->ctx)) == NULL) {
        TAL_PR_DEBUG("ERROR: failed to create WOLFSSL object\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("wolfSSL_new SUCCESS!\n");

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(client->ssl, client->sockfd)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to set the file descriptor\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("attach wolfSSL to the socket SUCCESS!\n");
    
    return ret;
}

int ws_server_connect(esp_websocket_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    esp_websocket_client_event_t ws_event;
    ws_event.client = client;
    ws_event.event_id = WEBSOCKET_EVENT_ON_CONNECTED;
    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(client->ssl)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to connect to wolfSSL error:%d\n", ret);
        ws_event.event_id = WEBSOCKET_EVENT_ERROR;
        client->event_handler(&ws_event);
        return ESP_ERR;
    }

    /* Sending Upgrade request */
    TAL_PR_DEBUG("Sending Upgrade Request to Server......\n");
    size_t bytes_rx_tx;
    if((bytes_rx_tx = wolfSSL_write(client->ssl, WS_UPGRADE_REQ, strlen(WS_UPGRADE_REQ))) != strlen(WS_UPGRADE_REQ)) {
        TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        TAL_PR_DEBUG("%d bytes of %d bytes were sent", bytes_rx_tx, (int) strlen(WS_UPGRADE_REQ));
        ws_event.event_id = WEBSOCKET_EVENT_ERROR;
        client->event_handler(&ws_event);
        return ESP_ERR;
    }

    ws_event.connection_upcall = client->connection_upcall;
    client->event_handler(&ws_event);
    TAL_PR_DEBUG("Connect to wolfSSL on the server side SUCCESS!\n");
    THREAD_CFG_T thrd_param = {4096, 4, "beken-ws-rcv"};
    tal_thread_create_and_start(&bk_ws_rcv_thread, NULL, NULL, ws_receive, client, &thrd_param);
    return ret;
}

void ws_receive(esp_websocket_client_handle_t client)
{
    uint8_t buf[RX_TX_BUFFER_SIZE];
	int len;
    int ret;
	TAL_PR_DEBUG( "  Started Receiving Thread\n" );
    esp_websocket_client_event_t ws_event;
	do
	{
		len = sizeof( buf ) - 1;
		memset( buf, 0, sizeof( buf ) );
		ret = wolfSSL_read( client->ssl, buf, len );

		if( ret == WOLFSSL_ERROR_WANT_READ || ret == WOLFSSL_ERROR_WANT_WRITE )
		{
		  continue;
		}
		if( ret < 0 )
		{
		  TAL_PR_DEBUG( "failed\n  ! wolfSSL_read returned %d\n\n", ret );
		  break;
		}

		if( ret == 0 )
		{
		  TAL_PR_DEBUG( "\n\nEOF\n\n" );
		  break;
		}

		len = ret;
        buf[len] = '\0'; // Ensure null-terminated string
		// TAL_PR_DEBUG( " %d bytes read:%s\n", len, buf );
        if(buf && len)
        {
            ws_event.event_id = WEBSOCKET_EVENT_ON_DATA;
            ws_event.data = buf;
            ws_event.data_len = len; 
            // ws_event.msg_upcall = client->msg_upcall;
            client->event_handler(&ws_event);
        }

	}while( 1 );
    esp_ws_client_cleanup(client);
	TAL_PR_DEBUG( "  Ended Receiving WS data\n" );

}

esp_err_t parse_url(esp_websocket_client_handle_t client, const char* url) 
{
    if (NULL == url) {
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate memory for the internal buffer and copy the URL
    size_t url_len = strlen(url);
    char* url_buffer = (char*)malloc(url_len + 1);
    if (url_buffer == NULL) {
        TAL_PR_ERR("ERROR: Unable to malloc url buffer");
        return ESP_ERR_NO_MEM;
    }
    strcpy(url_buffer, url);

    const char* url_start = url_buffer;
    TAL_PR_DEBUG("Url %s", url_start);
    const char* scheme_end = strstr(url_start, "://"); //wss|://hostname:port/path

    if (scheme_end != NULL) {
        // Move the start of the URL to after the scheme
        url_start = scheme_end + 3;  //wss://|hostname:port/path
    }

    // Find the position of the first '/' and colon after the hostname
    const char* slash = strchr(url_start, '/'); //wss://|hostname:port|/path
    const char* colon = strchr(url_start, ':'); //wss://|hostname|:port|/path

    TAL_PR_DEBUG("Host %s", url_start);

    size_t host_len;
    size_t port_len = 0;
    const char* path_start;

    if (colon != NULL && (slash == NULL || colon < slash)) {
        // Length of the host is the distance from the start of the URL to the colon
        host_len = colon - url_start;

        // Length of the port is the distance from the colon to the slash or end of the string
        port_len = (slash == NULL) ? strlen(colon + 1) : (size_t)(slash - colon - 1);

        // Allocate memory for host and port
        char* host = (char*)malloc(host_len + 1);
        char* port = (char*)malloc(port_len + 1);

        if (NULL == host || NULL == port) {
            TAL_PR_ERR("ERROR: Unable to malloc host and port");
            if (host) free(host);
            if (port) free(port);
            free(url_buffer);
            return ESP_ERR_NO_MEM;
        }

        // Copy the host and port
        strncpy(host, url_start, host_len);
        host[host_len] = '\0';  // Null-terminate the host string

        strncpy(port, colon + 1, port_len);
        port[port_len] = '\0';  // Null-terminate the port string

        // Assign to client struct
        client->host = host;
        client->port = port;
    } else {
        // No port provided
        host_len = (slash == NULL) ? strlen(url_start) : (size_t)(slash - url_start);

        char* host = (char*)malloc(host_len + 1);
        if (NULL == host) {
            TAL_PR_ERR("ERROR: Unable to malloc host");
            free(url_buffer);
            return ESP_ERR_NO_MEM;
        }

        strncpy(host, url_start, host_len);
        host[host_len] = '\0';  // Null-terminate the host string

        // Assign to client struct
        client->host = host;
        client->port = "80";  // Default port for HTTP
    }

    if (slash != NULL) {
        // Allocate memory for path
        path_start = slash;
        char* path = (char*)malloc(strlen(path_start) + 1);

        if (NULL == path) {
            TAL_PR_ERR("ERROR: Unable to malloc path");
            if (client->host) free((char*)client->host);
            if (client->port && client->port != "80") free((char*)client->port);
            free(url_buffer);
            return ESP_ERR_NO_MEM;
        }

        strcpy(path, path_start);  // Copy the path including the slash

        // Assign to client struct
        client->path = path;
    } else {
        client->path = strdup("/");  // Default path
        if (client->path == NULL) {
            TAL_PR_ERR("ERROR: Unable to malloc path");
            if (client->host) free((char*)client->host);
            if (client->port && client->port != "80") free((char*)client->port);
            free(url_buffer);
            return ESP_ERR_NO_MEM;
        }
    }

    // Print the results for debugging
    TAL_PR_DEBUG("Host: %s\n", client->host);
    TAL_PR_DEBUG("Port: %s\n", client->port);
    TAL_PR_DEBUG("Path: %s\n", client->path);

    // Free the url buffer after parsing
    free(url_buffer);

    return ESP_OK;
}
