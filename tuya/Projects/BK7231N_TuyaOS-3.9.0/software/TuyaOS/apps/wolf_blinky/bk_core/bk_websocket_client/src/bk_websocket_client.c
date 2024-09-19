#include "bk_websocket_client.h"
#include "esp_err.h"
#include "tal_log.h"
#include "tal_network.h"
#include "tal_thread.h"
#include "tkl_wifi.h"
#include "string.h"

/* freeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netdb.h"

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define TAL_PR_DEBUG
#define RX_TX_BUFFER_SIZE 256
#define WS_UPGRADE_REQ "GET / HTTP/1.1\r\nHost: cloud.ezlo.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-Websocket-Extensions: premessage-deflate; client_max_window_bits\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: chat, superchat\r\n\r\n"

THREAD_HANDLE bk_ws_rcv_thread = NULL;
THREAD_HANDLE bk_ws_ping_thread = NULL;
THREAD_HANDLE bk_ws_reconnect_thread = NULL;

TaskHandle_t bk_ws_rcv_thread_handle = NULL;

typedef enum {
    WEBSOCKET_STATE_UNINIT = 0,
    WEBSOCKET_STATE_INIT,
    WEBSOCKET_STATE_CONNECTED,
    WEBSOCKET_STATE_REQ_COMPLETE_HEADER,
    WEBSOCKET_STATE_REQ_COMPLETE_DATA,
    WEBSOCKET_STATE_RES_COMPLETE_HEADER,
    WEBSOCKET_STATE_RES_ON_DATA_START,
    WEBSOCKET_STATE_RES_COMPLETE_DATA,
    WEBSOCKET_STATE_RECONNECTING,
    WEBSOCKET_STATE_CLOSE
} esp_http_state_t;

// Enum for common HTTP status codes
typedef enum {
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NOT_FOUND = 404,
    // Add other status codes as needed
    HTTP_UNKNOWN = 0
} esp_http_status_code;


struct esp_websocket_client {
    const char* url;
    const char* host;
    const char* path;    
    const char* port;
    ws_event_handle_cb        event_handler;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    int sockfd;
    int state;
    void (*connection_upcall)(bool connected);
    void (*msg_upcall)(const char*, uint32_t)
};

typedef enum ws_transport_opcodes {
    WS_TRANSPORT_OPCODES_CONT =  0x00,
    WS_TRANSPORT_OPCODES_TEXT =  0x01,
    WS_TRANSPORT_OPCODES_BINARY = 0x02,
    WS_TRANSPORT_OPCODES_CLOSE = 0x08,
    WS_TRANSPORT_OPCODES_PING = 0x09,
    WS_TRANSPORT_OPCODES_PONG = 0x0a,
    WS_TRANSPORT_OPCODES_FIN = 0x80,
    WS_TRANSPORT_OPCODES_NONE = 0x100,   /*!< not a valid opcode to indicate no message previously received
                                          * from the API esp_transport_ws_get_read_opcode() */
} ws_transport_opcodes_t;

/* Prototypes */
void ws_receive(esp_websocket_client_handle_t client);
void ws_ping(esp_websocket_client_handle_t client);
esp_err_t parse_url(esp_websocket_client_handle_t client, const char* url);
int parse_http_response_code(const char *response);
int ws_send_websocket_message(WOLFSSL* ssl, const char* strdata, ws_transport_opcodes_t opcode);

esp_err_t esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char* data, int len, int delay)
{
    return ws_send_websocket_message(client->ssl, data, WS_TRANSPORT_OPCODES_TEXT);
}

esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config)
{
    esp_websocket_client_handle_t client;
    esp_err_t ret = ESP_OK;

    bool _success = (client = calloc(1, sizeof(esp_websocket_client_handle_t)));
    if (!_success) {
        TAL_PR_DEBUG("Error allocate memory");
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
    client->msg_upcall = config->msg_upcall;

    ret = parse_url(client, config->url);
    if(ret != ESP_OK)
    {
        TAL_PR_DEBUG("ERROR parse_url error num = %s", ret);
        return NULL;
    }

    if(ws_socket_initialize(client)!= ESP_OK)
    {
        TAL_PR_DEBUG("ERROR ws_socket_initialize error num = %s", ret);
        return NULL;
    }

    ret = (esp_err_t)ws_ssl_initialize(config, client);
    ret = (esp_err_t)ws_ssl_create(client);

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
    return ws_server_connect(client);
}

bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client)
{
    return (client->state == WEBSOCKET_STATE_CONNECTED);
}

void esp_ws_client_cleanup(esp_websocket_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    esp_websocket_client_event_t ws_event;
    /* Bidirectional shutdown */
    if (wolfSSL_shutdown(client->ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        TAL_PR_DEBUG("Shutdown not complete\n");
    }
    wolfSSL_free(client->ssl);      /* Free the wolfSSL object */
    client->ssl = NULL;

    client->state = WEBSOCKET_STATE_CLOSE;
    ws_event.connection_upcall = client->connection_upcall;
    ws_event.event_id = WEBSOCKET_EVENT_DISCONNECTED;
    client->event_handler(&ws_event);

    close(client->sockfd);    
    client->sockfd = -1;
    TAL_PR_DEBUG("Client is now NULL");

    return ret;
}

int ws_socket_initialize(esp_websocket_client_handle_t client)
{
    struct addrinfo hints, *result;
    size_t len;
    int ret = ESP_OK;

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

    /* set socket options */
    tal_net_set_timeout(client->sockfd, 5000, TRANS_RECV);

    /* Resolve domain name to IP address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(client->host, NULL, &hints, &result) != 0) {
        TAL_PR_ERR("ERROR: failed to resolve domain name %s\n", client->host);
        close(client->sockfd);
        return ESP_ERR;
    }

    /* Print the resolved IP address */
    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    addr->sin_port = atoi(client->port);
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

    return ret;
}

int ws_ssl_create(esp_websocket_client_handle_t client)
{
    int ret = ESP_OK;
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
    return ESP_OK;
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

    /* Check the response for upgrade request and then start the WS Recieve thread */
    char upgrade_req_res[256];
    while ((bytes_rx_tx = wolfSSL_read(client->ssl, upgrade_req_res, sizeof(upgrade_req_res)-1)) > 0) {
        upgrade_req_res[bytes_rx_tx] = '\0';  // Null-terminate the buffer
        if (strstr(upgrade_req_res, "\r\n\r\n") != NULL) {
            TAL_PR_DEBUG("Upgrade Request Response : %s", upgrade_req_res);
            break;
        }
    }
    int response_code = parse_http_response_code(upgrade_req_res);
    TAL_PR_DEBUG("Response Code : %d", response_code);
    if(response_code == HTTP_SWITCHING_PROTOCOLS)
    {
        ws_event.connection_upcall = client->connection_upcall;
        client->event_handler(&ws_event);
        client->state = WEBSOCKET_STATE_CONNECTED;
        TAL_PR_DEBUG("Connect to wolfSSL on the server side SUCCESS!\n");

        THREAD_CFG_T thrd_param = {4096, THREAD_PRIO_0, "beken-ws-rcv"};
        tal_thread_create_and_start(&bk_ws_rcv_thread, NULL, NULL, ws_receive, client, &thrd_param);

        ret = ESP_OK;
    }
    else
    {
        TAL_PR_ERR("Unable to upgrade connection to Websocket \nStatus code :%d\n", response_code);
        esp_ws_client_cleanup(client);
        ret = ESP_ERR;
    }

    return ret;
}

void ws_receive(esp_websocket_client_handle_t client)
{
    uint8_t buf[RX_TX_BUFFER_SIZE];
	int len;
    int ret;
	TAL_PR_DEBUG( "  Started Receiving Thread\n" );
    esp_websocket_client_event_t ws_event;
    WF_STATION_STAT_E status = -1;
	do
	{
		len = sizeof( buf ) - 1;
		memset( buf, 0, sizeof( buf ) );

        tal_wifi_station_get_status(&status);
        TAL_PR_DEBUG("WIFI_STATUS = %d", (int)status);

		ret = wolfSSL_read( client->ssl, buf, len );

		if( ret == WOLFSSL_ERROR_WANT_READ || ret == WOLFSSL_ERROR_WANT_WRITE )
		{
		  continue;
		}
		if( ret < 0 )
		{
		//   TAL_PR_DEBUG( "failed\n  ! wolfSSL_read returned %d\n\n", ret );
		  TAL_PR_DEBUG( "Nothing to WOLFSSL READ" );
          continue;
		//   break;
		}

		if( ret == 0 )
		{
		  TAL_PR_DEBUG( "\n\nEOF\n\n" );
		  break;
		}

		len = ret;
        buf[len] = '\0'; // Ensure null-terminated string
        if(buf && len)
        {
            ws_event.event_id = WEBSOCKET_EVENT_ON_DATA;
            ws_event.data = buf;
            ws_event.data_len = len; 
            ws_event.msg_upcall = client->msg_upcall;
            client->event_handler(&ws_event);
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
	}while((status == WSS_IDLE) || (status == WSS_CONN_SUCCESS) || (status == WSS_GOT_IP) );
	TAL_PR_DEBUG( "OUTSIDE %s status %d\n", __FUNCTION__, status);

    esp_ws_client_cleanup(client);
    esp_websocket_reconnect(client);
    tal_thread_delete(bk_ws_rcv_thread);
    bk_ws_rcv_thread = NULL;
	TAL_PR_DEBUG( "  Ended Receiving WS data\n" );

}

void ws_reconnect_task(esp_websocket_client_handle_t client)
{
    TAL_PR_DEBUG("  Started Reconnect Task\n");
    int ret = ESP_FAIL;
    esp_websocket_client_event_t ws_event;

    WF_STATION_STAT_E status = -1;

    // Wait until Wi-Fi is reconnected
    do {
        tal_wifi_station_get_status(&status);
        TAL_PR_DEBUG("Waiting for Wi-Fi to reconnect, current status: %d", (int)status);

        if (status == WSS_CONN_SUCCESS || status == WSS_GOT_IP || status == WSS_IDLE) {
            // Wi-Fi is reconnected, break the loop
            break;
        }

        // Delay before checking again
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } while (1);

    TAL_PR_DEBUG("Wi-Fi reconnected, reinitializing WolfSSL connection");

    ws_socket_initialize(client);

    ws_ssl_create(client);

    ws_server_connect(client);

    TAL_PR_DEBUG("Reconnect Task Completed\n");
    tal_thread_delete(bk_ws_reconnect_thread);
    bk_ws_reconnect_thread = NULL;

}


void esp_websocket_reconnect(esp_websocket_client_handle_t client)
{
    THREAD_CFG_T cfg = {4 * 2048, THREAD_PRIO_3, "ws-reconnect"};
    tal_thread_create_and_start(&bk_ws_reconnect_thread, NULL, NULL, ws_reconnect_task, client, &cfg);

}

void ws_ping(esp_websocket_client_handle_t client)
{
    while(esp_websocket_client_is_connected(client))
    {
        vTaskDelay(20000/portTICK_PERIOD_MS);
        TAL_PR_ERR("SEDNIGN PING");
        ws_send_websocket_message(client->ssl, "ping", WS_TRANSPORT_OPCODES_PING);
    }
    tal_thread_delete(bk_ws_ping_thread);
    bk_ws_ping_thread = NULL;

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

// Function to parse the HTTP response code
int parse_http_response_code(const char *response) {
    // Find the start of the response code
    const char *status_code_start = strchr(response, ' ') + 1;

    // Extract the response code as a string
    char status_code_str[4];
    strncpy(status_code_str, status_code_start, 3);
    status_code_str[3] = '\0'; // Null-terminate the string

    // Convert the status code string to an integer
    int status_code = atoi(status_code_str);

    return status_code;
}

int ws_send_websocket_message(WOLFSSL* ssl, const char* strdata, ws_transport_opcodes_t opcode)
{
    unsigned char mask[4];
	unsigned int mask_int;
	unsigned long long payload_len;
	unsigned char finNopcode;
	unsigned int payload_len_small;
	unsigned int payload_offset = 6;
	unsigned int len_size;
	unsigned long long be_payload_len;
	unsigned int sent = 0;
	int i, sockfd;
	unsigned int frame_size;
	char *data;
    int ret;


	finNopcode = 0x80 | opcode; //FIN and text opcode.
	mask_int = (unsigned int) 1234;
	memcpy(mask, &mask_int, 4);
	payload_len = strlen(strdata);
	if(payload_len <= 125) {
		frame_size = 6 + payload_len;
		payload_len_small = payload_len;

	} else if(payload_len > 125 && payload_len <= 0xffff) {
		frame_size = 8 + payload_len;
		payload_len_small = 126;
		payload_offset += 2;
	} else if(payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL) {
		frame_size = 14 + payload_len;
		payload_len_small = 127;
		payload_offset += 8;
	} else {
		return -1; //too large payload
	}
	data = (char *)malloc(frame_size);
	memset(data, 0, frame_size);
	*data = finNopcode;
	*(data+1) = payload_len_small | 0x80; //payload length with mask bit on
	if(payload_len_small == 126) {
		payload_len &= 0xffff;
		len_size = 2;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	if(payload_len_small == 127) {
		payload_len &= 0xffffffffffffffffLL;
		len_size = 8;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	for(i=0;i<4;i++)
		*(data+(payload_offset-4)+i) = mask[i];

	memcpy(data+payload_offset, strdata, strlen(strdata));
	for(i=0;i<strlen(strdata);i++)
		*(data+payload_offset+i) ^= mask[i % 4] & 0xff;
	sent = 0;
	i = 0;

    TAL_PR_DEBUG("Sending WebSocket message to Cloud Server......\n");
    if ((ret = wolfSSL_write(ssl, data, frame_size)) != frame_size) {
        TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        TAL_PR_DEBUG("%d bytes of %d bytes were sent\n", ret, (int)frame_size);
        free(data);
        return -1;
    }

    free(data);
    return ESP_OK;
}