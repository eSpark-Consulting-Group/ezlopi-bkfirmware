#include <stdlib.h>
#include "tal_log.h"
#include "tal_network.h"
#include "esp_http_client.h"
#include "bk_core_config.h"

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netdb.h"

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

/* turn off logs */
#ifndef CONFIG_ESP_HTTP_CLIENT_DEBUG
#define TAL_PR_DEBUG
#endif

#define RX_TX_BUFFER_SIZE 256

struct http_header_handle {
    void* buffer;
};

typedef struct http_header_handle http_header_handle_t;

/**
 * HTTP Buffer
 */
typedef struct {
    char *data;         /*!< The HTTP data received from the server */
    int len;            /*!< The HTTP data len received from the server */
    char *raw_data;     /*!< The HTTP data after decoding */
    char *orig_raw_data;/*!< The Original pointer to HTTP data after decoding */
    int raw_len;        /*!< The HTTP data len after decoding */
    char *output_ptr;   /*!< The destination address of the data to be copied to after decoding */
} esp_http_buffer_t;

/**
 * private HTTP Data structure
 */
typedef struct {
    http_header_handle_t headers;       /*!< http header */
    esp_http_buffer_t   *buffer;        /*!< data buffer as linked list */
    int                 status_code;    /*!< status code (integer) */
    int64_t             content_length; /*!< data length */
    int                 method;         /*!< http method */
} esp_http_data_t;

typedef enum {
    HTTP_STATE_UNINIT = 0,
    HTTP_STATE_INIT,
    HTTP_STATE_CONNECTED,
    HTTP_STATE_REQ_COMPLETE_HEADER,
    HTTP_STATE_REQ_COMPLETE_DATA,
    HTTP_STATE_RES_COMPLETE_HEADER,
    HTTP_STATE_RES_ON_DATA_START,
    HTTP_STATE_RES_COMPLETE_DATA,
    HTTP_STATE_CLOSE
} esp_http_state_t;

struct esp_http_client {
    const char* url;
    const char* host;
    const char *path;    
    esp_http_data_t *request;
    esp_http_data_t *response;
    http_event_handle_cb        event_handler;
    void* user_data;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    int sockfd;
    int state;
    int method;
};
typedef struct esp_http_client esp_http_client_t;
/* Private Funcition Prototypes */
void parse_headers(char *response, esp_http_client_handle_t client);

esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config)
{
    esp_http_client_handle_t client;
    esp_err_t ret = ESP_OK;
    bool _success = (
        (client                         = calloc(1, sizeof(esp_http_client_t)))           &&
        (client->request                = calloc(1, sizeof(esp_http_data_t)))             &&
        (client->request->buffer        = calloc(1, sizeof(esp_http_buffer_t)))           &&
        (client->response               = calloc(1, sizeof(esp_http_data_t)))             &&
        (client->response->buffer       = calloc(1, sizeof(esp_http_buffer_t)))
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
    
    if( NULL == (config->url || config->event_handler || config->user_data) )
    {
        TAL_PR_DEBUG("Error url or handler or");
        goto error;
    }
    client->url = config->url;
    client->event_handler = config->event_handler;
    client->user_data = config->user_data;  

    if(NULL != client->url)
    {
        // Skip the protocol (e.g., "https://")
        const char* start_of_host = strstr(config->url, "://");
        if (start_of_host != NULL) 
        {
            start_of_host += 3;  // Move past "://"
        } 
        else 
        {
            start_of_host = config->url;  // No protocol found, assume the URL starts with the host
        }

        // Find the position of the first '/'
        const char* slash = strchr(start_of_host, '/');
        
        if (slash != NULL) 
        {
            TAL_PR_DEBUG("Slash found started parsing!\n");
            // Length of the host is the distance from the start of the host to the slash
            size_t host_len = slash - start_of_host;  
            
            // Allocate memory for host and path
            client->host = (char*)malloc(host_len + 1);
            client->path = (char*)malloc(strlen(slash) + 1);  // The path is from the slash to the end of the URL
            if(NULL == client->host || NULL == client->path)
            {
                TAL_PR_ERR("ERROR: Unable to malloc path and host");
                goto error;
            }
            
            // Copy the host and path
            strncpy((char*)client->host, start_of_host, host_len);
            ((char*)client->host)[host_len] = '\0';  // Null-terminate the host string

            strcpy((char*)client->path, slash);    // Copy the path including the slash
            
            // Print the results for debugging
            TAL_PR_DEBUG("Host: %s\n", client->host);
            TAL_PR_DEBUG("Path: %s\n", client->path);
        } 
        else 
        {
            size_t url_len = strlen(start_of_host);
            TAL_PR_DEBUG("Invalid URL format or path not found!\n");
            client->host = (char*)malloc(url_len + 1);
            strncpy((char*)client->host, start_of_host, url_len);
            ((char*)client->host)[url_len] = '\0';  // Null-terminate the host string
            client->path = (char*)malloc(2 * sizeof(char));
            strcpy((char*)client->path, "/");

            // Print the results for debugging
            TAL_PR_DEBUG("Host: %s\n", client->host);
            TAL_PR_DEBUG("Path: %s\n", client->path);
        }
    }

    socket_initialize(config, client);
    ssl_initialize(config, client);

    if(ret == ESP_OK)
    {
        return client;
    }

error:
    esp_http_client_cleanup(client);
    return ESP_ERR;

}

esp_err_t esp_http_client_perform(esp_http_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    switch (client->state)
    {
    case HTTP_STATE_INIT:
        ret = server_connect(client);
        /* Falls through */
    case HTTP_STATE_CONNECTED:
        ret = esp_http_client_request_send(client);
        /* Falls through */
    case HTTP_STATE_REQ_COMPLETE_HEADER:   

        /* Falls through */
    case HTTP_STATE_REQ_COMPLETE_DATA:

        /* Falls through */
    case HTTP_STATE_RES_COMPLETE_HEADER:

        /* Falls through */
    case HTTP_STATE_RES_ON_DATA_START:

        /* Falls through */
    case HTTP_STATE_RES_COMPLETE_DATA:

        /* Falls through */
    case HTTP_STATE_CLOSE:
        break;
    default:
        break;
    }

}

int esp_http_client_get_status_code(esp_http_client_handle_t client)
{
    return client->response->status_code;
}

bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client)
{

}

esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    esp_http_client_event_t http_event;
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

    http_event.event_id = HTTP_EVENT_DISCONNECTED;
    client->event_handler(&http_event);

    close(client->sockfd);    
    return ret;
}

esp_err_t esp_http_client_set_method(esp_http_client_handle_t client, esp_http_client_method_t method)
{
    client->method = method;
    return ESP_OK;
}

/* -------------------------------------------------------------------- */

int socket_initialize(const esp_http_client_config_t *config, esp_http_client_handle_t client)
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

int ssl_initialize(esp_http_client_config_t *config, esp_http_client_handle_t client)
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
    if (wolfSSL_CTX_use_PrivateKey_buffer(client->ctx, config->client_key_pem, strlen(config->client_key_pem), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG(stderr, "Error loading client private key\n");
        return ESP_ERR;
    }
    TAL_PR_DEBUG("\t PRIVATE KEY LOADED\n");

    // Load client's certificate
    if (wolfSSL_CTX_use_certificate_buffer(client->ctx, config->client_cert_pem, strlen(config->client_cert_pem), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
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
    
    client->state = HTTP_STATE_INIT; 
    return ret;
}

int server_connect(esp_http_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    esp_http_client_event_t http_event;
    http_event.client = client;
    http_event.event_id = HTTP_EVENT_ON_CONNECTED;
    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(client->ssl)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to connect to wolfSSL error:%d\n", ret);
        http_event.event_id = HTTP_EVENT_ERROR;
        client->event_handler(&http_event);
        return ESP_ERR;
    }
    client->event_handler(&http_event);
    TAL_PR_DEBUG("Connect to wolfSSL on the server side SUCCESS!\n");
    client->state = HTTP_STATE_CONNECTED; 
    return ret;
}

int esp_http_client_request_send(esp_http_client_handle_t client)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_rx_tx;
    esp_http_client_event_t http_event;
    char* buf_ptr = client->request->buffer->data;
    int totalRead = 0;

    http_event.client = client;
    switch (client->method)
    {
    case HTTP_METHOD_GET:
        client->request->buffer->len = sprintf(buf_ptr, "GET %s %s\r\nHost: %s\r\n\r\n", client->path, "HTTP/1.1", client->host);
        break;
    case HTTP_METHOD_POST:
        client->request->buffer->data = "{\"hello\":\"world\"}\r\n\r\n"; //FIXME:
        client->request->buffer->len = sprintf(buf_ptr, "POST %s %s\r\nHost: %s\r\nContent-Type: %s\r\nnContent-Length: %d\r\n\r\n%s", client->path, "HTTP/1.1", "application/json", strlen(client->request->buffer->data), client->host, client->request->buffer->data);
        break;
    default:
        http_event.event_id = HTTP_EVENT_ERROR;
        client->event_handler(&http_event);
        break;
    }

    TAL_PR_DEBUG("SENDING REQUEST %s", buf_ptr);
    if ((bytes_rx_tx = wolfSSL_write(client->ssl, client->request->buffer->data, client->request->buffer->len)) != client->request->buffer->len) {
        TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        TAL_PR_DEBUG("%d bytes of %d bytes were sent\n", bytes_rx_tx, (int)client->request->buffer->len);
        free(client->request->buffer->data);
        http_event.event_id = HTTP_EVENT_ERROR;
        client->event_handler(&http_event);
        ret = ESP_ERR;
        return ret;
    }
    http_event.event_id = HTTP_EVENT_HEADERS_SENT;
    client->event_handler(&http_event);

    /* Reading Response Headers */
    while (1) {
        memset(buf_ptr, 0, RX_TX_BUFFER_SIZE);
        bytes_rx_tx = wolfSSL_read(client->ssl, buf_ptr, RX_TX_BUFFER_SIZE - 1);
        if (bytes_rx_tx <= 0) {

            TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
            http_event.event_id = HTTP_EVENT_ERROR;
            client->event_handler(&http_event);
            ret = ESP_ERR;
            return ret;
        }

        /* Check if we've reached the end of the headers */
        if (strstr(buf_ptr, "\r\n\r\n") != NULL) {
            parse_headers(buf_ptr, client);
            if(client->response->status_code != 200)
            {
                http_event.event_id = HTTP_EVENT_ERROR;
                client->event_handler(&http_event);
            }
                http_event.event_id = HTTP_EVENT_ON_HEADER;
                http_event.data = buf_ptr;
                http_event.data_len = strlen(buf_ptr);
                client->event_handler(&http_event);
            break;
        }
        
    }
    /* Read the response body */
    totalRead = 0;
    while ((bytes_rx_tx = wolfSSL_read(client->ssl, buf_ptr, RX_TX_BUFFER_SIZE - 1)) > 0) {
        buf_ptr[bytes_rx_tx] = '\0';  // Null-terminate the buffer
        // TAL_PR_DEBUG("%s", buf_ptr);
        http_event.event_id = HTTP_EVENT_ON_DATA;
        http_event.data = buf_ptr;
        http_event.data_len = strlen(buf_ptr);
        http_event.user_data = client->user_data;
        client->event_handler(&http_event);
        totalRead += bytes_rx_tx;
    }

    http_event.event_id = HTTP_EVENT_ON_FINISH;
    client->event_handler(&http_event);
    
    if (bytes_rx_tx < 0) {
        TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
    }
}

// Function to parse headers and save content length and status code
void parse_headers(char *response, esp_http_client_handle_t client) 
{
    char *header_start = response;
    char *header_end;

    // Extract status code from the status line
    char *status_line_end = strstr(header_start, "\r\n");
    if (status_line_end) {
        *status_line_end = '\0';  // Null-terminate the status line

        // Find the status code in the status line
        char *status_code_start = strchr(header_start, ' ') + 1;
        if (status_code_start) {
            char *status_code_end = strchr(status_code_start, ' ');
            if (status_code_end) {
                *status_code_end = '\0';  // Null-terminate the status code
                client->response->status_code = atoi(status_code_start);
            }
        }

        // Move header_start past the end of the status line
        header_start = status_line_end + 2;
    }

    while ((header_end = strstr(header_start, "\r\n")) != NULL) {
        *header_end = '\0';  // Null-terminate the header line

        // Find the colon separating key and value
        char *colon_pos = strstr(header_start, ": ");
        if (colon_pos) {
            *colon_pos = '\0';  // Null-terminate the key
            char *key = header_start;
            char *value = colon_pos + 2;  // Value starts after ": "

            // Check if the key is "Content-Length"
            if (strcasecmp(key, "Content-Length") == 0) {
                client->response->content_length = atoi(value);
            }
        }

        // Move to the next header line
        header_start = header_end + 2;
    }
}

#if 0
// Function to parse headers and save content length and status code
void parse_headers(char *response, esp_http_client_event_t *http_event, esp_http_client_handle_t client) {
    char *header_start = response;
    char *header_end;

    // Extract status code from the status line
    char *status_line_end = strstr(header_start, "\r\n");
    if (status_line_end) {
        *status_line_end = '\0';  // Null-terminate the status line

        // Find the status code in the status line
        char *status_code_start = strchr(header_start, ' ') + 1;
        if (status_code_start) {
            char *status_code_end = strchr(status_code_start, ' ');
            if (status_code_end) {
                *status_code_end = '\0';  // Null-terminate the status code
                client->response->status_code = atoi(status_code_start);
            }
        }

        // Move header_start past the end of the status line
        header_start = status_line_end + 2;
    }

    while ((header_end = strstr(header_start, "\r\n")) != NULL) {
        *header_end = '\0';  // Null-terminate the header line

        // Find the colon separating key and value
        char *colon_pos = strstr(header_start, ": ");
        if (colon_pos) {
            *colon_pos = '\0';  // Null-terminate the key
            char *key = header_start;
            char *value = colon_pos + 2;  // Value starts after ": "

            // Allocate memory and copy the key and value
            http_event->header_key = (char *)malloc(strlen(key) + 1);
            http_event->header_value = (char *)malloc(strlen(value) + 1);
            if (http_event->header_key && http_event->header_value) {
                strcpy(http_event->header_key, key);
                strcpy(http_event->header_value, value);

                // Check if the key is "Content-Length"
                if (strcasecmp(key, "Content-Length") == 0) {
                    client->response->content_length = atoi(value);
                }

                // Trigger the event for this header
                http_event->event_id = HTTP_EVENT_ON_HEADER;
                http_event->data = header_start;
                http_event->data_len = strlen(header_start);
                client->event_handler(http_event);

                // Free the allocated memory after the event handler has processed the headers
                free(http_event->header_key);
                free(http_event->header_value);
            }
        }

        // Move to the next header line
        header_start = header_end + 2;
    }
}
#endif