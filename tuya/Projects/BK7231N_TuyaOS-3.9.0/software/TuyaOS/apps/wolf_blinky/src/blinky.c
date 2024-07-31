/**
 * @file tuya_app_main.c
 * @author www.tuya.com
 * @brief tuya_app_main module is used to 
 * @version 0.1
 * @date 2022-10-28
 *
 * @copyright Copyright (c) tuya.inc 2022
 *
 */

#include <stdlib.h>
#include "tal_log.h"
#include "uni_log.h"
#include "tkl_gpio.h"
#include "tal_uart.h"
#include "tal_wifi.h"
#include "tkl_output.h"
#include "tal_thread.h"
#include "tal_system.h"
#include "tuya_ws_db.h"
#include "tuya_cloud_types.h"
#include "tuya_iot_com_api.h"
#include "ezlopi_http_client.h"
#include "cJSON.h"
#include "tal_memory.h"

#ifdef ENABLE_WIFI_SERVICE
#include "tuya_iot_wifi_api.h"
#else
#include "tuya_iot_base_api.h"
#endif
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
#include "lwip_init.h"
#endif

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netdb.h"

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

// #include "esp_http_client.h"
#include "ezlopi_http_client.h"
#include "ezlopi_core_websocket.h"
#include "ezlopi_cloud_registration.h"
#include "ezlopi_core_api_methods.h"
#include "ezlopi_cloud_keywords.h"
#include "ezlopi_service_webprov.h"

#include "cert.h"
#include "cJSON.h"

// #define USE_CERT_BUFFERS_1024
// #include "wolfssl/certs_test.h"
#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26

THREAD_HANDLE ty_app_thread = NULL;
THREAD_HANDLE bk_rcv_thread = NULL;
#define SERVER_PORT 443
// #define GET_REQUEST "GET /getserver?json=true HTTP/1.1\r\n\r\n"
// #define GET_REQUEST "GET /getserver?json=true HTTP/1.1\r\nHost: cloud.ezlo.com:7000\r\nAccept-Encoding: identity\r\n\r\n"

// #define GET_REQUEST "\x81\x87{\"serial\":\"102003333\",\"firmware\":\"4.1.1\",\"timeOffset\":20700,\"media\":\"radio\",\"hubType\":\"32.1\",\"controller_uuid\":\"a7ee12e0-8394-11ee-8889-0f56091e7639\",\"mac_address\":\"08:F9:E0:E4:22:34\",\"maxFrameSize\":20480}"
// #define WS_REGISTER_REQ "{ \"method\":\"register\", \"id\":\"_ID_\", \"params\": { \"serial\":\"102003333\",\"firmware\":\"3.8.9\",\"timeOffset\":20700,\"media\":\"radio\",\"hubType\":\"32.1\",\"controller_uuid\":\"a7ee12e0-8394-11ee-8889-0f56091e7639\",\"mac_address\":\"08:F9:E0:E4:22:34\",\"maxFrameSize\":20480} }"
#define WS_REGISTER_REQ "{\"method\":\"register\",\"id\":\"_ID_\",\"params\":{\"serial\":\"102003333\",\"hubType\":\"32.1\",\"controller_uuid\":\"a7ee12e0-8394-11ee-8889-0f56091e7639\",\"mac_address\":\"08:F9:E0:E4:22:34\"}}"
#define WS_UPGRADE_REQ "GET / HTTP/1.1\r\nHost: nma-server6-cloud.ezlo.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-Websocket-Extensions: premessage-deflate; client_max_window_bits\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: chat, superchat\r\n\r\n"
#define WS_ITEM_LIST "{\"result\":{\"items\":[{\"_id\":\"20e46000\",\"deviceId\":\"10e46000\",\"hasGetter\":true,\"hasSetter\":true,\"name\":\"switch\",\"show\":true,\"valueType\":\"bool\",\"value\":false,\"valueFormatted\":\"false\",\"status\":\"idle\"}]},\"error\":null,\"id\":\"_ID_\",\"sender\":{\"conn_id\":\"a7ee12e0-8394-11ee-8889-0f56091e7639\",\"type\":\"ui\"},\"method\":\"hub.items.list\",\"msg_id\":18}"
#define WS_DEVICE_LIST "{\"result\":{\"devices\":[{\"_id\":\"10e46000\",\"deviceTypeId\":\"ezlopi\",\"parentDeviceId\":\"\",\"category\":\"switch\",\"subcategory\":\"in_wall\",\"gatewayId\":\"457a5069\",\"batteryPowered\":false,\"name\":\"FrankEverSwitch\",\"type\":\"switch.inwall\",\"reachable\":true,\"persistent\":true,\"serviceNotification\":false,\"armed\":false,\"roomId\":\"\",\"security\":\"no\",\"ready\":true,\"status\":\"idle\"}]},\"error\":null,\"id\":\"_ID_\",\"sender\":{\"conn_id\":\"a7ee12e0-8394-11ee-8889-0f56091e7639\",\"type\":\"ui\"},\"method\":\"hub.devices.list\",\"msg_id\":16}\"
#define WS_MTHD_NOTFOUND "{\"error\":{\"code\":-32602,\"data\":\"rpc.method.notfound\",\"message\":\"Unknownmethod\"},\"result\":{},\"method\":\"hub.devices.get\",\"msg_id\":16,}"

// Define byte order conversion functions if not available
#ifndef htobe64
uint64_t htobe64(uint64_t host_64bits) {
    uint64_t result = ((uint64_t)htons((uint16_t)((host_64bits >> 48) & 0xFFFF)) << 48) |
                      ((uint64_t)htons((uint16_t)((host_64bits >> 32) & 0xFFFF)) << 32) |
                      ((uint64_t)htons((uint16_t)((host_64bits >> 16) & 0xFFFF)) << 16) |
                      ((uint64_t)htons((uint16_t)(host_64bits & 0xFFFF)));
    return result;
}
#endif

static int message_counter = 0;
typedef enum e_trace_type
{
    TRACE_TYPE_NONE = 0,
    TRACE_TYPE_B,
    TRACE_TYPE_I,
    TRACE_TYPE_E
} e_trace_type_t;

// #define WS_REGISTER_REQ "hello\r\n"
/* FUNC PROTO */
void call_wolf_funcs();
int send_websocket_message(WOLFSSL* ssl, const char* strdata);
void receive(WOLFSSL* ssl);
char* handle_message(const char* message);
s_ezlopi_http_data_t* ezlopi_http_get_request(const char* cloud_url, const char* private_key, const char* shared_key, const char* ca_certificate);
static esp_err_t ezlopi_http_event_handler(esp_http_client_event_t* evt);
static void __connection_upcall(bool connected);
static void __message_upcall(const char* payload, uint32_t len);

void initialize_responses();
STATIC VOID __free_heap_task(PVOID_T args);

STATIC THREAD_HANDLE free_heap_thrd = NULL;

STATIC VOID user_main(VOID_T)
{
        OPERATE_RET rt = OPRT_OK;

    /* LwIP Initialization */
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    TUYA_LwIP_Init();
#endif

    TUYA_GPIO_NUM_E pin_id = LED_PIN;
    TUYA_CALL_ERR_LOG(tuya_iot_init("./"));
    TUYA_GPIO_BASE_CFG_T led_cfg = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_LOW
    };
    TUYA_CALL_ERR_LOG(tkl_gpio_init(pin_id, &led_cfg));
    UINT32_T blink_interval = 1000;

    // uint8_t turnOn[] = "Led Turned On\r\n";
    // uint8_t turnOff[] = "Led Turned Off\r\n";


    // TUYA_CALL_ERR_LOG(tuya_cli_init_with_uart(TUYA_UART_NUM_0));
    INT_T result = OPRT_OK;
    TAL_UART_CFG_T cfg = {0};
    cfg.base_cfg.baudrate = 115200;
    cfg.base_cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    cfg.base_cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    cfg.base_cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    cfg.rx_buffer_size = 256;
    cfg.open_mode = O_BLOCK;
    result = tal_uart_init(TUYA_UART_NUM_0, &cfg);
    if (OPRT_OK != result) {
        TAL_PR_ERR("uart init failed", result);
    }

    TAL_PR_DEBUG("hello Ash-----");
    THREAD_CFG_T thread_cfg = 
    {
        .thrdname = "free_heap",
        .stackDepth = 1024,
        .priority = THREAD_PRIO_5,
    };
    if (NULL == free_heap_thrd) 
    {
        tal_thread_create_and_start(&free_heap_thrd, NULL, NULL, __free_heap_task, NULL, &thread_cfg);
    }
    // while(1)
    // {
    //     uint8_t turnOn[] = "Led Turned On\r\n";
    //     // tal_uart_write(TUYA_UART_NUM_0, turnOn, sizeof(turnOn));
    //     tal_system_sleep(1000);
    // }


    ezlopi_wifi_initialize();
    while (!ezlopi_wifi_is_connected()) 
    {  
        ezlopi_wifi_connect_from_id_bin();
        TAL_PR_ERR("Not Connected");
        tal_system_sleep(1000);
    }
    TAL_PR_ERR("Wifi Connected");
    // initialize_responses();

    // char user_http_req[128];
    // char* cloud_server = "cloud.ezlo.com";
    // snprintf(user_http_req, sizeof(user_http_req), "%s/getserver?json=true", cloud_server);
    // TAL_PR_DEBUG("http_request: %s", user_http_req);
    // s_ezlopi_http_data_t * ws_endpoint = ezlopi_http_get_request("cloud.ezlo.com/getserver?json=true", SSL_PRIVATE_KEY, SHARED_KEY, CA_CERT);
    // TAL_PR_DEBUG(">>>>>>>>>>>>>> wss endpoint : %s", ws_endpoint->response);
    // cJSON* root = cJSON_Parse(ws_endpoint->response);
    // if (root)
    // {
    //     cJSON* cjson_uri = cJSON_GetObjectItem(root, "uri");
    //     if (cjson_uri)
    //     {
    //         TAL_PR_DEBUG("uri: %s", cjson_uri->valuestring ? cjson_uri->valuestring : "NULL");
    //         // ezlopi_core_ezlopi_broadcast_method_add(__send_str_data_to_nma_websocket, "nma-websocket", 4);
    //         ezlopi_websocket_client_init(cjson_uri, __message_upcall, __connection_upcall);
    //         // task_complete = 1;
    //     }

    //     cJSON_Delete(root);
    // }
    

    // call_wolf_funcs();

    ezlopi_service_web_provisioning_init();
    
    while(1) {  
        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_HIGH);
        // tal_uart_write(TUYA_UART_NUM_0, turnOn, sizeof(turnOn));
        tal_system_sleep(blink_interval);

        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_LOW);
        tal_log_set_manage_attr(TAL_LOG_LEVEL_DEBUG);
        // tal_uart_write(TUYA_UART_NUM_0, turnOff, sizeof(turnOff));
        tal_system_sleep(blink_interval);

	    // Set logging level to DEBUG
        // TAL_PR_DEBUG("Hello TuyaOS.");
    }

    return;
}

STATIC VOID tuya_app_thread(VOID_T *arg )
{
    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

VOID_T tuya_app_main(VOID)
{
    THREAD_CFG_T thrd_param = {4096*2, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}

void call_wolf_funcs()
{
    int sockfd;
    struct addrinfo hints, *result;
    char buff[256];
    size_t len;
    int ret = -1;

    wolfSSL_Debugging_ON();
    wolfSSL_SetLoggingCb(tal_log_print);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        TAL_PR_ERR("ERROR: failed to create the socket\n");
        return;
    }

    TAL_PR_DEBUG("SOCKET CREATE SUCCESS!\n");

    /* Resolve domain name to IP address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("nma-server9-cloud.ezlo.com", NULL, &hints, &result) != 0) {
        TAL_PR_ERR("ERROR: failed to resolve domain name\n");
        close(sockfd);
        return;
    }

    /* Print the resolved IP address */
    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    addr->sin_port = SERVER_PORT;
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
    // inet_pton(AF_INET, &(addr->sin_addr), ipstr);
    TAL_PR_DEBUG("Resolved IP address: %s\n", ipstr);

    /* Connect to the server */
    if(tal_net_connect(sockfd, tal_net_str2addr(ipstr), addr->sin_port) < 0) {
        TAL_PR_ERR("ERROR: failed to connect to server\n");
        close(sockfd);
        freeaddrinfo(result);
        return;
    }

    TAL_PR_DEBUG("SOCKET CONNECT SUCCESS!\n");


    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to initialize the library\n");
        // goto socket_cleanup;
        return;
    }
    TAL_PR_DEBUG("SSL INIT SUCCESS!\n");


    /* Create and initialize WOLFSSL_CTX */
#ifdef WOLFSSL_TLS13
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
#else
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    // ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
#endif
    if (ctx == NULL) {
        TAL_PR_DEBUG("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        return;
    }
    TAL_PR_DEBUG("wolfSSL_CTX_new SUCCESS!\n");

        /* Loading keys */
    // Load client's private key

    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, SSL_PRIVATE_KEY, strlen(SSL_PRIVATE_KEY), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG(stderr, "Error loading client private key\n");
        return 1;
    }
    TAL_PR_DEBUG("\t PRIVATE KEY LOADED\n");

    // Load client's certificate
    if (wolfSSL_CTX_use_certificate_buffer(ctx, SHARED_KEY, strlen(SHARED_KEY), WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG(stderr, "Error loading client certificate\n");
        return 1;
    }
    TAL_PR_DEBUG("\t SHARED KEY LOADED\n");

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_buffer(ctx, CA_CERT, strlen(CA_CERT), WOLFSSL_FILETYPE_PEM))
         != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to load %s, please check the Cert. err: %d\n",CA_CERT, ret);
        return;
    }
    
    TAL_PR_DEBUG("wolfSSL_CTX_load_verify_buffer SUCCESS!\n");

        /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        TAL_PR_DEBUG("ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        return;
    }
    TAL_PR_DEBUG("wolfSSL_new SUCCESS!\n");

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to set the file descriptor\n");
    }
    TAL_PR_DEBUG("attach wolfSSL to the socket SUCCESS!\n");
    

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to connect to wolfSSL error:%d\n", ret);
        return -1;
    }
    TAL_PR_DEBUG("Connect to wolfSSL on the server side SUCCESS!\n");

    /* Send the message to the server */
    #define GET_REQUEST WS_UPGRADE_REQ
    TAL_PR_DEBUG("Sending Request to Server......\n");
    if ((ret = wolfSSL_write(ssl, GET_REQUEST, strlen(GET_REQUEST))) != strlen(GET_REQUEST)) {
        TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        TAL_PR_DEBUG("%d bytes of %d bytes were sent", ret, (int) strlen(GET_REQUEST));
    }

    // int totalRead = 0;

    // while (1) {
    //     memset(buff, 0, sizeof(buff));
    //     ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
    //     if (ret <= 0) {
    //         TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
    //         return;
    //     }
    //     TAL_PR_DEBUG("%s", buff);

    //     /* Check if we've reached the end of the headers */
    //     if (strstr(buff, "\r\n\r\n") != NULL) {
    //         break;
    //     }
    // }

    /* Read the response body */
    // TAL_PR_DEBUG("\nResponse Body:\n");
    // while ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
    //     buff[ret] = '\0';  // Null-terminate the buffer
    //     TAL_PR_DEBUG("%s", buff);
    //     totalRead += ret;
    // }

    // if (ret < 0) {
    //     TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
    // }


    /* Print to stdout any data the server sends */
    // TAL_PR_DEBUG("Server: %s\n", buff);

    /* Check Status Code */
    // const char *status_start = strstr(buff, "HTTP/1.1 ");
    // char status_code[4] = "404";
    // if (status_start) {
    //     // Move the pointer to the start of the status code
    //     status_start += strlen("HTTP/1.1 ");
    //     // Copy the status code (3 characters) into a new string
    //     strncpy(status_code, status_start, 3);
    //     status_code[3] = '\0'; // Null-terminate the string

    //     // Convert the status code to an integer
    //     int status = atoi(status_code);

    //     // Print the status code
    //     TAL_PR_DEBUG("Status code: %d\n", status);
    // } else {
    //     TAL_PR_DEBUG("Failed to find the status code in the response.\n");
    // }

    // char buffer_array[1024];
    // buffer_array[0] = 0x81;
    // int frame_len = strlen(WS_UPGRADE_REQ);
    // buffer_array[1] = frame_len;
    // strcpy(&buffer_array[14], WS_UPGRADE_REQ);
    // int frame_size = 1 + frame_len;

    // if(strcmp(status_code, "101") == 0)
    // {   
        // TAL_PR_DEBUG("Sending Register Request to Cloud Server......\n");
        // #define GET_REQUEST buffer_array
        // if ((ret = wolfSSL_write(ssl, GET_REQUEST, frame_size)) != frame_size) {
        //     TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        //     TAL_PR_DEBUG("%d bytes of %d bytes were sent", ret, (int) frame_size);
        // }
        
        // /* Read the response body */
        // while (1) {
        //     TAL_PR_DEBUG("In while .\n");
        //     memset(buff, 0, sizeof(buff));
        //     ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        //     if (ret <= 0) {
        //         TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
        //         return;
        //     }
        //     TAL_PR_DEBUG("%s", buff);

        //     /* Check if we've reached the end of the headers */
        //     if (strstr(buff, "\r\n\r\n") != NULL) {
        //         break;
        //     }
        // }
        // TAL_PR_DEBUG("Out while .\n");

        // if (ret < 0) {
        //     TAL_PR_DEBUG(stderr, "ERROR: failed to read\n");
        // }


    //     /* Print to stdout any data the server sends */
    //     TAL_PR_DEBUG("Server: %s\n", buff);
    // }
    send_websocket_message(ssl, WS_REGISTER_REQ);
    THREAD_CFG_T thrd_param = {4096, 4, "beken-ws-rcv"};
    tal_thread_create_and_start(&bk_rcv_thread, NULL, NULL, receive, ssl, &thrd_param);
    while(1)
    {
        tal_system_sleep(5000);
        TAL_PR_DEBUG("websocket is running");
        // send_websocket_message(ssl, WS_ITEM_LIST);
    }

    /* Bidirectional shutdown */
    // while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
    //     TAL_PR_DEBUG("Shutdown not complete\n");
    // }

    TAL_PR_DEBUG("Shutdown complete\n");

    ret = 0;

    /* Cleanup and return */
cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
// end:
}


int send_websocket_message(WOLFSSL* ssl, const char* strdata) {
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


	finNopcode = 0x81; //FIN and text opcode.
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

    // // Read response
    // char buff[1024];
    // // while (1) {
    //     TAL_PR_DEBUG("In while.\n");
    //     memset(buff, 0, sizeof(buff));
    //     ret = wolfSSL_read(ssl, buff, sizeof(buff) - 1);
    //     if (ret <= 0) {
    //         TAL_PR_DEBUG("ERROR: failed to read\n");
    //         return -1;
    //     }
    //     TAL_PR_DEBUG("%s\n", buff);

    //     // Check if we've reached the end of the headers
    //     // if (strstr(buff, "\r\n\r\n") != NULL) {
    //     //     break;
    //     // }
    // // }
    // TAL_PR_DEBUG("Out of while.\n");

    // if (ret < 0) {
    //     TAL_PR_DEBUG("ERROR: failed to read\n");
    // }
    // receive(ssl);

    return 0;
}

static uint8_t buf[1024];
void receive(WOLFSSL* ssl)
{
	int len;
    int ret;
	TAL_PR_DEBUG( "  Started Receiving Thread\n" );
	do
	{
		len = sizeof( buf ) - 1;
		memset( buf, 0, sizeof( buf ) );
		ret = wolfSSL_read( ssl, buf, len );

		if( ret == WOLFSSL_ERROR_WANT_READ || ret == WOLFSSL_ERROR_WANT_WRITE )
		{
		  continue;
		}

		if( ret == 0 )
		{
		  break;
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
        // buf[len] = '\0'; // Ensure null-terminated string
		TAL_PR_DEBUG( " %d bytes read:%s\n", len, buf );

        cJSON *json = cJSON_Parse((const char *)buf);
        if (json == NULL)
        {
            TAL_PR_DEBUG("Error parsing JSON\n");
        }
        else
        {
            cJSON *method = cJSON_GetObjectItemCaseSensitive(json, "method");
            if (cJSON_IsString(method) && (method->valuestring != NULL))
            {
                TAL_PR_DEBUG("Method: %s\n", method->valuestring);
                // if(strcmp(method->valuestring, "hub.devices.list"))
                // {
                //     send_websocket_message(ssl, WS_MTHD_NOTFOUND);
                // }
                // if(strcmp(method->valuestring, "hub.items.list"))
                // {
                //     send_websocket_message(ssl, WS_ITEM_LIST);
                // }
                // else
                // {
                //     send_websocket_message(ssl, WS_MTHD_NOTFOUND);
                // }
                send_websocket_message(ssl, handle_message(buf));
            }
            else
            {
                TAL_PR_DEBUG("Method key not found or not a string\n");
            }
            cJSON_Delete(json);
        }


	}while( 1 );
	TAL_PR_DEBUG( "  Ended Receiving WS data\n" );

}

/* --- - - - -- - handle response ---_- -- - - - */

// Define a structure for the response
// Structure for the response data
typedef struct {
    char id[50];
    int msg_id;
    char method[50];
    cJSON *result;
    cJSON *error;
} Response;

// Define the array size
#define NUM_RESPONSES 12

// Predeclare the response array
Response RESPONSES[NUM_RESPONSES];

#define NUM_RESPONSES (sizeof(RESPONSES) / sizeof(RESPONSES[0]))

void initialize_responses() {
    // hub.data.list
    strcpy(RESPONSES[0].method, "hub.data.list");
    RESPONSES[0].msg_id = 1;
    RESPONSES[0].error = cJSON_Parse("{\"code\": -32602, \"data\": \"rpc.method.notfound\", \"message\": \"Unknown method\"}");
    RESPONSES[0].result = cJSON_CreateObject();

    // hub.network.get
    strcpy(RESPONSES[1].method, "hub.network.get");
    RESPONSES[1].msg_id = 2;
    RESPONSES[1].result = cJSON_Parse("{\"interfaces\": [{\"_id\": \"wlan0\", \"enabled\": \"auto\", \"hwaddr\": \"c8:f0:9e:ac:a1:18\", \"internetAvailable\": true, \"ipv4\": {\"dns\": [\"192.168.1.254\"], \"ip\": \"192.168.1.101\", \"mask\": \"255.255.255.0\", \"gateway\": \"192.168.1.254\", \"mode\": \"dhcp\"}, \"priority\": 0, \"network\": \"wan\", \"status\": \"up\", \"type\": \"wifi\", \"wifi\": {\"mode\": \"sta\", \"network\": {\"bssid\": \"04:75:f9:1d:15:35\", \"encryption\": \"psk2\", \"key\": \"NDS_0ffice\", \"ssid\": \"nepaldigisys_2\"}, \"channel\": \"10\", \"region\": \"00\"}}]}");
    RESPONSES[1].error = cJSON_CreateNull();

    // hub.devices.list
    strcpy(RESPONSES[2].method, "hub.devices.list");
    RESPONSES[2].msg_id = 5;
    RESPONSES[2].result = cJSON_Parse("{\"devices\": [{\"_id\": \"10676000\", \"deviceTypeId\": \"ezlopi\", \"parentDeviceId\": \"\", \"category\": \"switch\", \"subcategory\": \"relay\", \"gatewayId\": \"457a5069\", \"batteryPowered\": false, \"name\": \"beken_switch_temp\", \"type\": \"switch.inwall\", \"reachable\": true, \"persistent\": true, \"serviceNotification\": false, \"armed\": false, \"roomId\": \"\", \"security\": \"no\", \"ready\": true, \"status\": \"idle\"}, {\"_id\": \"10676001\", \"deviceTypeId\": \"ezlopi\", \"parentDeviceId\": \"\", \"category\": \"dimmable_light\", \"subcategory\": \"dimmable_bulb\", \"gatewayId\": \"457a5069\", \"batteryPowered\": false, \"name\": \"Dimmer\", \"type\": \"dimmer.outlet\", \"reachable\": true, \"persistent\": true, \"serviceNotification\": false, \"armed\": false, \"roomId\": \"\", \"security\": \"no\", \"ready\": true, \"status\": \"idle\"}, {\"_id\": \"10676002\", \"deviceTypeId\": \"ezlopi\", \"parentDeviceId\": \"\", \"category\": \"temperature\", \"subcategory\": \"\", \"gatewayId\": \"457a5069\", \"batteryPowered\": false, \"name\": \"beken_DHT22_temp_humi_temp\", \"type\": \"sensor\", \"reachable\": true, \"persistent\": true, \"serviceNotification\": false, \"armed\": false, \"roomId\": \"\", \"security\": \"no\", \"ready\": true, \"status\": \"idle\"}, {\"_id\": \"10676003\", \"deviceTypeId\": \"ezlopi\", \"parentDeviceId\": \"10676002\", \"category\": \"humidity\", \"subcategory\": \"\", \"gatewayId\": \"457a5069\", \"batteryPowered\": false, \"name\": \"beken_DHT22_temp_humi_humi\", \"type\": \"sensor\", \"reachable\": true, \"persistent\": true, \"serviceNotification\": false, \"armed\": false, \"roomId\": \"\", \"security\": \"no\", \"ready\": true, \"status\": \"idle\"}]}");
    RESPONSES[2].error = cJSON_CreateNull();

    // hub.items.list
    strcpy(RESPONSES[3].method, "hub.items.list");
    RESPONSES[3].msg_id = 6;
    RESPONSES[3].result = cJSON_Parse("{\"items\": [{\"_id\": \"20676000\", \"deviceId\": \"10676000\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"switch\", \"show\": true, \"valueType\": \"bool\", \"value\": false, \"valueFormatted\": \"false\", \"status\": \"idle\"}, {\"_id\": \"20676001\", \"deviceId\": \"10676001\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"dimmer\", \"show\": true, \"valueType\": \"int\", \"minValue\": 0, \"maxValue\": 100, \"value\": 0, \"valueFormatted\": \"0\", \"status\": \"idle\"}, {\"_id\": \"20676002\", \"deviceId\": \"10676001\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"dimmer_up\", \"show\": true, \"valueType\": \"int\", \"value\": 0, \"valueFormatted\": \"0\", \"status\": \"idle\"}, {\"_id\": \"20676003\", \"deviceId\": \"10676001\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"dimmer_down\", \"show\": true, \"valueType\": \"int\", \"value\": 0, \"valueFormatted\": \"0\", \"status\": \"idle\"}, {\"_id\": \"20676004\", \"deviceId\": \"10676001\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"dimmer_stop\", \"show\": true, \"valueType\": \"int\", \"value\": 0, \"valueFormatted\": \"0\", \"status\": \"idle\"}, {\"_id\": \"20676005\", \"deviceId\": \"10676001\", \"hasGetter\": true, \"hasSetter\": true, \"name\": \"switch\", \"show\": true, \"valueType\": \"bool\", \"value\": false, \"valueFormatted\": \"false\", \"status\": \"idle\"}, {\"_id\": \"20676006\", \"deviceId\": \"10676002\", \"hasGetter\": true, \"hasSetter\": false, \"name\": \"temp\", \"show\": true, \"valueType\": \"temperature\", \"scale\": \"celsius\", \"value\": 0, \"valueFormatted\": \"0.00\", \"status\": \"idle\"}, {\"_id\": \"20676007\", \"deviceId\": \"10676003\", \"hasGetter\": true, \"hasSetter\": false, \"name\": \"humidity\", \"show\": true, \"valueType\": \"humidity\", \"scale\": \"percent\", \"value\": 0, \"valueFormatted\": \"0.00\", \"status\": \"idle\"}]}");
    RESPONSES[3].error = cJSON_CreateNull();

    // hub.scenes.list
    strcpy(RESPONSES[4].method, "hub.scenes.list");
    RESPONSES[4].msg_id = 7;
    RESPONSES[4].error = cJSON_Parse("{\"code\": -32602, \"data\": \"rpc.method.notfound\", \"message\": \"Unknown method\"}");
    RESPONSES[4].result = cJSON_CreateObject();

    // hub.room.list
    strcpy(RESPONSES[5].method, "hub.room.list");
    RESPONSES[5].msg_id = 8;
    RESPONSES[5].result = cJSON_CreateObject();
    RESPONSES[5].error = cJSON_CreateNull();

    // hub.info.get
    strcpy(RESPONSES[6].method, "hub.info.get");
    RESPONSES[6].msg_id = 9;
    RESPONSES[6].result = cJSON_Parse("{\"model\": \"ezlopi_generic\", \"architecture\": \"xtensa-esp32s2\", \"chipId\": \"esp32\", \"serial\": 105200113, \"uuid\": \"623d3d65-0795-47df-b954-6e5f3baf13fc\", \"offlineAnonymousAccess\": true, \"offlineInsecureAccess\": true, \"location\": {\"latitude\": 0, \"longitude\": 0, \"timezone\": \"GMT0\", \"state\": \"default\"}, \"build\": {\"time\": \"2024-07-10T14:42:27+0000\", \"builder\": \"lomas.subedi@ezlo.com\", \"branch\": \"development\", \"commit\": \"c2fbb27a518900e391a060655358b9f06429b0c0\"}, \"battery\": {\"stateOfCharge\": 0, \"remainingTime\": 0, \"health\": 0, \"status\": \"\"}, \"localtime\": \"2024-07-18T09:10:03+0000\", \"uptime\": \"0d 0h 0m 53s\"}");
    RESPONSES[6].error = cJSON_CreateNull();

    // hub.features.list
    strcpy(RESPONSES[7].method, "hub.features.list");
    RESPONSES[7].msg_id = 10;
    RESPONSES[7].error = cJSON_Parse("{\"code\": -32602, \"data\": \"rpc.method.notfound\", \"message\": \"Unknown method\"}");
    RESPONSES[7].result = cJSON_CreateObject();

    // hub.custom.nodes.list
    strcpy(RESPONSES[8].method, "hub.custom.nodes.list");
    RESPONSES[8].msg_id = 11;
    RESPONSES[8].error = cJSON_Parse("{\"code\": -32602, \"data\": \"rpc.method.notfound\", \"message\": \"Unknown method\"}");
    RESPONSES[8].result = cJSON_CreateObject();

    // hub.modes.get
    strcpy(RESPONSES[9].method, "hub.modes.get");
    RESPONSES[9].msg_id = 16;
    RESPONSES[9].error = cJSON_Parse("{\"code\": -32602, \"data\": \"rpc.method.notfound\", \"message\": \"Unknown method\"}");
    RESPONSES[9].result = cJSON_CreateObject();
}


// Function to find a predefined response by method
Response* get_response_by_method(const char* method) {
    for (int i = 0; i < NUM_RESPONSES; i++) {
        if (strcmp(RESPONSES[i].method, method) == 0) {
            return &RESPONSES[i];
        }
    }
    return NULL;
}

char* handle_message(const char* message) {
    cJSON* data = cJSON_Parse(message);
    if (!data) {
        TAL_PR_DEBUG("Error parsing JSON\n");
        return NULL;
    }

    TAL_PR_DEBUG("WS Request: %s\n", cJSON_Print(data));

    cJSON* method = cJSON_GetObjectItem(data, "method");
    cJSON* id = cJSON_GetObjectItem(data, "id");
    cJSON* sender = cJSON_GetObjectItem(data, "sender");

    if (!cJSON_IsString(method) || !method->valuestring || !cJSON_IsString(id) || !id->valuestring) {
        TAL_PR_DEBUG("Invalid method or id\n");
        cJSON_Delete(data);
        return NULL;
    }

    Response* response = get_response_by_method(method->valuestring);
    if (response) {
        strncpy(response->id, id->valuestring, sizeof(response->id) - 1);
        response->msg_id += 1;

        cJSON* response_json = cJSON_CreateObject();
        cJSON_AddStringToObject(response_json, "id", response->id);
        cJSON_AddStringToObject(response_json, "method", response->method);
        cJSON_AddNumberToObject(response_json, "msg_id", response->msg_id);

        if (response->result) {
            cJSON_AddItemToObject(response_json, "result", cJSON_Duplicate(response->result, 1));
        }
        if (response->error) {
            cJSON_AddItemToObject(response_json, "error", cJSON_Duplicate(response->error, 1));
        }

        if (sender && cJSON_IsObject(sender)) {
            cJSON_AddItemToObject(response_json, "sender", cJSON_Duplicate(sender, 1));
        }

        char* response_str = cJSON_Print(response_json);
        TAL_PR_DEBUG("WS Response: %s\n", response_str);
        cJSON_Delete(response_json);
        cJSON_Delete(data);
        return response_str;
    } else {
        cJSON* error_response = cJSON_CreateObject();
        cJSON_AddStringToObject(error_response, "id", id->valuestring);
        cJSON_AddStringToObject(error_response, "method", method->valuestring);
        cJSON* error = cJSON_CreateObject();
        cJSON_AddNumberToObject(error, "code", -32602);
        cJSON_AddStringToObject(error, "data", "rpc.method.notfound");
        cJSON_AddStringToObject(error, "message", "Unknown method");
        cJSON_AddItemToObject(error_response, "error", error);
        cJSON_AddItemToObject(error_response, "result", cJSON_CreateObject());

        char* error_str = cJSON_Print(error_response);
        TAL_PR_DEBUG("WS Error Response: %s\n", error_str);
        cJSON_Delete(error_response);
        cJSON_Delete(data);
        return error_str;
    }
}

STATIC VOID __free_heap_task(PVOID_T args)
{
    for (;;) {
        TAL_PR_NOTICE("free heap: %d", tal_system_get_free_heap_size());
        tal_system_sleep(3000);
    }
}

// static void __connection_upcall(bool connected)
// {
//     TAL_PR_DEBUG("wss-connection: %s", connected ? "connected." : "disconnected!");
//     static int prev_status; // 0: never connected, 1: Not-connected, 2: connected
//     if (connected)
//     {
//         if (0 == prev_status)
//         {
//             TAL_PR_DEBUG("Web-socket Connected.");
//         }
//         else
//         {
//             TAL_PR_DEBUG("Web-socket Re-connected.");
//         }

//         prev_status = 2;
//         TAL_PR_DEBUG("Starting registration process....");
//         registration_init();
//     }
//     else
//     {
//         prev_status = 1;
//         // ezlopi_event_group_clear_event(EZLOPI_EVENT_NMA_REG);
//     }
// }

// static void __message_upcall(const char* payload, uint32_t len)
// {
//     TAL_PR_DEBUG("In Message UpCall");
//     cJSON *cj_request = cJSON_ParseWithLength(payload, len);

//     if (cj_request)
//     {
//         cJSON *cj_error = cJSON_GetObjectItem(cj_request, "error");
//         cJSON *cj_method = cJSON_GetObjectItem(cj_request, "method");

//         if ((NULL == cj_error) || (NULL == cj_error->valuestring) || (0 == strncmp(cj_error->valuestring, "null", 4)))
//         {
//             if ((NULL != cj_method) && (NULL != cj_method->valuestring))
//             {
//                 TAL_PR_DEBUG("## WS Rx <<<<<<<<<< '%s'\r\n%.*s", (cj_method->valuestring ? cj_method->valuestring : ""), len, payload);
//                 uint32_t method_idx = ezlopi_core_ezlopi_methods_search_in_list(cj_method);
//                 if (UINT32_MAX != method_idx)
//                 {
//                     __call_method_func_and_send_response(cj_request, cj_method, method_list_v2[method_idx].method, TRACE_TYPE_B);
//                     __call_method_func_and_send_response(cj_request, cj_method, method_list_v2[method_idx].updater, TRACE_TYPE_B);
//                 }
//                 // else
//                 // {
//                 //     __call_method_func_and_send_response(cj_request, cj_method, __rpc_method_notfound, TRACE_TYPE_E);
//                 // }
//             }
//         }
//         else
//         {
//             TAL_PR_DEBUG("## WS Rx <<<<<<<<<<'%s'\r\n%.*s", (cj_method->valuestring ? cj_method->valuestring : ""), len, payload);
//         }

//         cJSON_Delete(cj_request);
//     }
//     // TAL_PR_DEBUG("## WS Rx <<<<<<<<<<'%s'\r\n", (cj_method->valuestring ? cj_method->valuestring : ""));
// }

// void __call_method_func_and_send_response(cJSON *cj_request, cJSON *cj_method, f_method_func_t method_func, e_trace_type_t print_type)
// {
//     if (method_func)
//     {
//         if (registered == method_func)
//         {
//             method_func(cj_request, NULL);
//         }
//         else
//         {
//             cJSON *cj_response = cJSON_CreateObject();
//             if (NULL != cj_response)
//             {
//                 cJSON *cj_sender = cJSON_GetObjectItem(cj_request, ezlopi_sender_str);

//                 cJSON_AddNumberToObject(cj_response, ezlopi_msg_id_str, message_counter);
//                 cJSON_AddItemReferenceToObject(cj_response, ezlopi_sender_str, cj_sender);
//                 cJSON_AddNullToObject(cj_response, "error");

//                 method_func(cj_request, cj_response);

//                 // web_provisioning_send_to_nma_websocket(cj_response, print_type);
//                 ezlopi_websocket_client_send(cj_response, strlen(cj_response));
//                 cJSON_Delete(cj_response);
//             }
//             else
//             {
//                 TAL_PR_DEBUG("Error - cj_response: %d", (uint32_t)cj_response);
//             }
//         }
//     }
// }