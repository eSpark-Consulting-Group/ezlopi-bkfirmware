#include "string.h"
#include "uni_base64.h"
#include "uni_random.h"
#include "tal_log.h"
#include "tal_network.h"
#include "tal_thread.h"
#include "tkl_wifi.h"
#include "esp_err.h"
#include "bk_websocket_client.h"
#include "bk_core_config.h"
#include "tal_system.h"

/* Socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netdb.h"

/* WolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#ifndef CONFIG_BK_WEBSOCKET_CLIENT_DEBUG
#define TAL_PR_DEBUG
#endif // !CONFIG_BK_WEBSOCKET_CLIENT_DEBUG

#define RX_TX_BUFFER_SIZE 256
#define UPGRADE_BUFF_LEN 512

#define WS_UPGRADE_REQ "GET / HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-Websocket-Extensions: premessage-deflate; client_max_window_bits\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: chat, superchat\r\n\r\n"

/* Thread handles */
static THREAD_HANDLE bk_ws_rcv_thread = NULL;
static THREAD_HANDLE bk_ws_ping_thread = NULL;
static THREAD_HANDLE bk_ws_reconnect_thread = NULL;
WF_STATION_STAT_E status = 0;


typedef enum {
    WEBSOCKET_STATE_UNINIT = 0,
    WEBSOCKET_STATE_INIT,
    WEBSOCKET_STATE_CONNECTED,
    WEBSOCKET_STATE_SSL_CREATED,
    WEBSOCKET_STATE_REQ_COMPLETE_HEADER,
    WEBSOCKET_STATE_REQ_COMPLETE_DATA,
    WEBSOCKET_STATE_RES_COMPLETE_HEADER,
    WEBSOCKET_STATE_RES_ON_DATA_START,
    WEBSOCKET_STATE_RES_COMPLETE_DATA,
    WEBSOCKET_STATE_RECONNECTING,
    WEBSOCKET_STATE_CLOSE
} esp_http_state_t;

typedef enum {
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NOT_FOUND = 404,
    HTTP_UNKNOWN = 0
} esp_http_status_code_t;

typedef struct esp_websocket_client {
    const char* url;
    const char* host;
    const char* path;    
    const char* port;
    ws_event_handle_cb event_handler;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    int sockfd;
    int state;
    void (*connection_upcall)(bool connected);
    void (*msg_upcall)(const char*, uint32_t);
} esp_websocket_client_t;

/* WebSocket opcodes */
typedef enum ws_transport_opcodes {
    WS_TRANSPORT_OPCODES_CONT =  0x00,
    WS_TRANSPORT_OPCODES_TEXT =  0x01,
    WS_TRANSPORT_OPCODES_BINARY = 0x02,
    WS_TRANSPORT_OPCODES_CLOSE = 0x08,
    WS_TRANSPORT_OPCODES_PING = 0x09,
    WS_TRANSPORT_OPCODES_PONG = 0x0a,
    WS_TRANSPORT_OPCODES_FIN = 0x80,
    WS_TRANSPORT_OPCODES_NONE = 0x100,
} ws_transport_opcodes_t;

/* Prototypes */
static void ws_receive(esp_websocket_client_handle_t client);
static void ws_ping(esp_websocket_client_handle_t client);
static esp_err_t parse_url(esp_websocket_client_handle_t client, const char* url);
static int parse_http_response_code(const char *response);
static int ws_send_websocket_message(WOLFSSL* ssl, const char* strdata, ws_transport_opcodes_t opcode);
static void esp_ws_client_cleanup(esp_websocket_client_handle_t client);
static void esp_websocket_reconnect(esp_websocket_client_handle_t client);
static int ws_socket_initialize(esp_websocket_client_handle_t client);
static int ws_ssl_initialize(const esp_websocket_client_config_t *config, esp_websocket_client_handle_t client);
static int ws_ssl_create(esp_websocket_client_handle_t client);
static int ws_server_connect(esp_websocket_client_handle_t client);
static void ws_reconnect_task(esp_websocket_client_handle_t client);

esp_err_t esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char* data, int len, int delay)
{
    return ws_send_websocket_message(client->ssl, data, WS_TRANSPORT_OPCODES_TEXT);
}

esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config)
{
    esp_websocket_client_handle_t client = NULL;

    if (!config || !config->url || !config->event_handler) {
        TAL_PR_DEBUG("Error: URL or event handler is NULL");
        return NULL;
    }

    client = calloc(1, sizeof(esp_websocket_client_t));
    if (!client) {
        TAL_PR_DEBUG("Error: Memory allocation failed");
        return NULL;
    }
    
    client->url = config->url;
    client->event_handler = config->event_handler;
    client->connection_upcall = config->connection_upcall;
    client->msg_upcall = config->msg_upcall;

    if (parse_url(client, config->url) != ESP_OK) {
        TAL_PR_DEBUG("Error: Failed to parse URL");
        esp_ws_client_cleanup(client);
        return NULL;
    }

    if (ws_socket_initialize(client) != ESP_OK) {
        TAL_PR_DEBUG("Error: Failed to initialize socket");
        esp_ws_client_cleanup(client);
        return NULL;
    }

    if (ws_ssl_initialize(config, client) != ESP_OK || ws_ssl_create(client) != ESP_OK) {
        TAL_PR_DEBUG("Error: Failed to initialize SSL");
        esp_ws_client_cleanup(client);
        return NULL;
    }

    return client;
}

int esp_websocket_client_start(esp_websocket_client_handle_t client)
{
    return ws_server_connect(client);
}

bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client)
{
    return (client->state == WEBSOCKET_STATE_CONNECTED);
}

static void esp_ws_client_cleanup(esp_websocket_client_handle_t client)
{
    if (!client) {
        return;
    }

    if (client->ssl) {
        if (wolfSSL_shutdown(client->ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
            TAL_PR_DEBUG("Warning: Shutdown not complete");
        }
        wolfSSL_free(client->ssl);
        client->ssl = NULL;
    }

    client->state = WEBSOCKET_STATE_CLOSE;
    if (client->event_handler) {
        esp_websocket_client_event_t ws_event = {
            .connection_upcall = client->connection_upcall,
            .event_id = WEBSOCKET_EVENT_DISCONNECTED,
        };
        client->event_handler(&ws_event);
    }

    if (client->sockfd >= 0) {
        close(client->sockfd);
        client->sockfd = -1;
    }
    TAL_PR_DEBUG("Client cleanup completed");
}

static int ws_socket_initialize(esp_websocket_client_handle_t client)
{
    struct addrinfo hints = {0}, *result;
    int ret = ESP_OK;

    client->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client->sockfd == -1) {
        TAL_PR_ERR("Error: Failed to create the socket");
        return ESP_ERR;
    }

    TAL_PR_DEBUG("Socket creation success");

    tal_net_set_timeout(client->sockfd, 5000, TRANS_RECV);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(client->host, NULL, &hints, &result) != 0) {
        TAL_PR_ERR("Error: Failed to resolve domain name %s", client->host);
        close(client->sockfd);
        return ESP_ERR;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    addr->sin_port = atoi(client->port);
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
    TAL_PR_DEBUG("Resolved IP address: %s", ipstr);

    if (tal_net_connect(client->sockfd, tal_net_str2addr(ipstr), addr->sin_port) < 0) {
        TAL_PR_ERR("Error: Failed to connect to server");
        close(client->sockfd);
        freeaddrinfo(result);
        return ESP_ERR;
    }

    TAL_PR_DEBUG("Socket connect success");
    client->state = WEBSOCKET_STATE_INIT;
    freeaddrinfo(result);
    return ret;
}

static int ws_ssl_initialize(const esp_websocket_client_config_t *config, esp_websocket_client_handle_t client)
{
    wolfSSL_Debugging_ON();
    wolfSSL_SetLoggingCb(tal_log_print);

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("Error: Failed to initialize the WolfSSL library");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("SSL initialization success");

#ifdef WOLFSSL_TLS13
    client->ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
#else
    client->ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
#endif
    if (client->ctx == NULL) {
        TAL_PR_DEBUG("Error: Failed to create WolfSSL context");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("WolfSSL context creation success");

    if (wolfSSL_CTX_use_PrivateKey_buffer(client->ctx, config->client_key, strlen(config->client_key), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("Error: Failed to load client private key");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("Private key loaded");

    if (wolfSSL_CTX_use_certificate_buffer(client->ctx, config->client_cert, strlen(config->client_cert), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("Error: Failed to load client certificate");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("Client certificate loaded");

    if (wolfSSL_CTX_load_verify_buffer(client->ctx, config->cert_pem, strlen(config->cert_pem), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("Error: Failed to load server certificate");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("Server certificate loaded");

    return ESP_OK;
}

static int ws_ssl_create(esp_websocket_client_handle_t client)
{
    client->ssl = wolfSSL_new(client->ctx);
    int ret = WOLFSSL_FAILURE;

    if (client->ssl == NULL) {
        TAL_PR_ERR("Error: Failed to create SSL object");
        return ESP_ERR;
    }

    ret = wolfSSL_set_fd(client->ssl, client->sockfd);
    if(ret !=  WOLFSSL_SUCCESS)
    {
        TAL_PR_ERR("Error : SSL set fd failed with error code %d,", ret);
        wolfSSL_free(client->ssl);
        return ESP_ERR;
    }

    ret = wolfSSL_connect(client->ssl);
    if ( ret != WOLFSSL_SUCCESS) {
        TAL_PR_ERR("Error: SSL connect failed with error code %d,", ret);
        wolfSSL_free(client->ssl);
        return ESP_ERR;
    }

    TAL_PR_DEBUG("SSL connection success");
    client->state = WEBSOCKET_STATE_SSL_CREATED;
    return ESP_OK;
}

char* _generate_base64_encoded_key()
{
    // Calculate the size of the base64 buffer
    int binlength = 16; // Change this to the desired length of random data
    int base64_buf_size = TY_BASE64_BUF_LEN_CALC(binlength);
    unsigned char *bindata = (unsigned char *)calloc(0, binlength);

    // uni_random_string(bindata, binlength);
    bindata = "hello world!";  //TODO: Currently its not random, getting random string sometimes work and sometimes not.

    // Allocate memory for the base64 buffer
    char *base64 = (char *)malloc(base64_buf_size);
    if (base64 == NULL) {
        TAL_PR_DEBUG("Memory allocation failed\n");
        free(bindata);
        return 1;
    }

    tuya_base64_encode(bindata, base64, binlength);
    char* _key = base64; 

    // Clean up
    free(bindata);
    return _key;
}

static int ws_server_connect(esp_websocket_client_handle_t client)
{
    int ret = ESP_OK;
    esp_websocket_client_event_t ws_event;
    ws_event.client = client;
    ws_event.connection_upcall = client->connection_upcall;


    char recv_buf[RX_TX_BUFFER_SIZE];
    char upgrade_buf[UPGRADE_BUFF_LEN];

    memset(recv_buf, 0, sizeof(recv_buf));
    memset(upgrade_buf, 0, sizeof(upgrade_buf));

    // char* random_key = _generate_base64_encoded_key();
    char* random_key = (char*)malloc(sizeof(char)*24);
    random_key = "dGhlIHNhbXBsZSBub25jZQ==";

    snprintf(upgrade_buf, UPGRADE_BUFF_LEN, WS_UPGRADE_REQ, client->host, random_key);
    TAL_PR_DEBUG("%s", upgrade_buf); 

    if (wolfSSL_write(client->ssl, upgrade_buf, strlen(upgrade_buf)) < 0) {
        TAL_PR_ERR("Error: Failed to send WebSocket upgrade request");
        free(random_key);
        return ESP_ERR;
    }
    free(random_key);

    memset(recv_buf, 0, sizeof(recv_buf));
    int bytes_received = wolfSSL_read(client->ssl, recv_buf, sizeof(recv_buf) - 1);
    if (bytes_received < 0) {
        TAL_PR_ERR("Error: Failed to receive response");
        return ESP_ERR;
    }

    recv_buf[bytes_received] = '\0';
    int status_code = parse_http_response_code(recv_buf);
    TAL_PR_DEBUG("Status Code %d", status_code);
    if (status_code != HTTP_SWITCHING_PROTOCOLS) {
        TAL_PR_ERR("Error: WebSocket handshake failed, status code: %d", status_code);
        return ESP_ERR;
    }

    TAL_PR_DEBUG("WebSocket handshake success");
    client->state = WEBSOCKET_STATE_CONNECTED;
    ws_event.event_id = WEBSOCKET_EVENT_ON_CONNECTED;
    client->event_handler(&ws_event);

    TAL_PR_DEBUG("Creating receive thread...");
    THREAD_CFG_T thrd_param = {4096, THREAD_PRIO_0, "beken-ws-rcv"};
    tal_thread_create_and_start(&bk_ws_rcv_thread, NULL, NULL, ws_receive, client, &thrd_param);
    return ret;
}

static int parse_http_response_code(const char *response)
{
    int status_code = HTTP_UNKNOWN;
    sscanf(response, "HTTP/1.1 %d", &status_code);
    return status_code;
}

static void ws_receive(esp_websocket_client_handle_t client)
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
        
        status = 0;
        tal_wifi_station_get_status(&status);
        TAL_PR_DEBUG("WIFI_STATUS = %d", (int)status);

		ret = wolfSSL_read( client->ssl, buf, len );

		if( ret == WOLFSSL_ERROR_WANT_READ || ret == WOLFSSL_ERROR_WANT_WRITE )
		{
		  continue;
		}
		if( ret < 0 )
		{
		  TAL_PR_DEBUG( "Nothing to WOLFSSL READ" );
          continue;
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

static void ws_ping(esp_websocket_client_handle_t client)
{
    while (true) {
        ws_send_websocket_message(client->ssl, "ping", WS_TRANSPORT_OPCODES_PING);
        vTaskDelay(pdMS_TO_TICKS(10000)); // Ping every 10 seconds
    }
}

static void esp_websocket_reconnect(esp_websocket_client_handle_t client)
{
    THREAD_CFG_T cfg = {4 * 2048, THREAD_PRIO_3, "ws-reconnect"};
    tal_thread_create_and_start(&bk_ws_reconnect_thread, NULL, NULL, ws_reconnect_task, client, &cfg);
}

static void ws_reconnect_task(esp_websocket_client_handle_t client)
{
    TAL_PR_DEBUG("  Started Reconnect Task\n");
    int ret = ESP_FAIL;
    esp_websocket_client_event_t ws_event;
    int reconnect_retries = 0;

    // Wait until Wi-Fi is reconnected
    do {
        status = 0;
        tal_wifi_station_get_status(&status);
        TAL_PR_DEBUG("Waiting for Wi-Fi to reconnect, current status: %d", (int)status);

        if (status == WSS_CONN_SUCCESS || status == WSS_GOT_IP || status == WSS_IDLE) {
            // Wi-Fi is reconnected, break the loop
            break;
        }
       // Delay before checking again
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } while (1);

    TAL_PR_DEBUG("Wi-Fi reconnected, reinitializing WolfSSL connection, Client State = %d", client->state);

    while (client->state != WEBSOCKET_STATE_CONNECTED)
    {
        if(reconnect_retries >= 10)
        {
            tal_system_reset();  //FIXME: Need to fix the reconnect routine, works fine without bluetooth, probably due to insufficient heap
        }

        vTaskDelay(pdMS_TO_TICKS(200));
        switch (client->state)
        {
        case WEBSOCKET_STATE_CLOSE:
            if ((ret = ws_socket_initialize(client)) != OPRT_OK)
            {
                TAL_PR_ERR("Socket initialization failed");
                reconnect_retries++;
                break;
            }
            reconnect_retries = 0;
            break;
        case WEBSOCKET_STATE_INIT:
            if ((ret = ws_ssl_create(client)) != OPRT_OK)
            {
                TAL_PR_ERR("SSL creation failed with error");
                reconnect_retries++;
                break;
            }
            reconnect_retries = 0;
            break;
        case WEBSOCKET_STATE_SSL_CREATED:
            if ((ret = ws_server_connect(client)) != OPRT_OK)
            {
                TAL_PR_ERR("Server connection failed");
                reconnect_retries++;
                break;
            }
            reconnect_retries = 0;
            break;       
        
        default:
            break;
        }

    }


    TAL_PR_DEBUG("Reconnect Task Completed\n");
    tal_thread_delete(bk_ws_reconnect_thread);
    bk_ws_reconnect_thread = NULL;

}

static int ws_send_websocket_message(WOLFSSL* ssl, const char* strdata, ws_transport_opcodes_t opcode)
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

static esp_err_t parse_url(esp_websocket_client_handle_t client, const char* url) 
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
