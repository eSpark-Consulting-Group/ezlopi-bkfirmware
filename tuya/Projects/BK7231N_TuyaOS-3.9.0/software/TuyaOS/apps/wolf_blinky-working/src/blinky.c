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

#include "cert.h"
// #define USE_CERT_BUFFERS_1024
// #include "wolfssl/certs_test.h"
#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26

THREAD_HANDLE ty_app_thread = NULL;

#define GET_REQUEST "GET /getserver HTTP/1.1\r\nHost: cloud.ezlo.com:7000\r\nUser-Agent: python-requests/2.22.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n"

/* FUNC PROTO */
void call_wolf_funcs();

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
    // while(1)
    // {
    //     uint8_t turnOn[] = "Led Turned On\r\n";
    //     // tal_uart_write(TUYA_UART_NUM_0, turnOn, sizeof(turnOn));
    //     tal_system_sleep(1000);
    // }

    ezlopi_wifi_initialize();
    ezlopi_wifi_connect_from_id_bin();

    while (!ezlopi_wifi_is_connected()) {
        TAL_PR_ERR("Not Connected");
        tal_system_sleep(1000);
    }
    TAL_PR_ERR("Wifi Connected");

    // is_network_initialized();
    // ezlopi_http_send_get_request("https://cloud.ezlo.com:7000/getserver?json=true", NULL);
    // ezlopi_http_send_get_request("http://httpbin.org/", NULL);

    call_wolf_funcs();
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
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
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

    if (getaddrinfo("cloud.ezlo.com", NULL, &hints, &result) != 0) {
        TAL_PR_ERR("ERROR: failed to resolve domain name\n");
        close(sockfd);
        return;
    }

    /* Print the resolved IP address */
    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    addr->sin_port = 443;
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);
    // inet_pton(AF_INET, &(addr->sin_addr), ipstr);
    TAL_PR_DEBUG("Resolved IP address: %s\n", ipstr);

    /* Connect to the server */
    // if (connect(sockfd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
    if(tal_net_connect(sockfd, tal_net_str2addr(ipstr), addr->sin_port) < 0) {
        TAL_PR_ERR("ERROR: failed to connect to server\n");
        close(sockfd);
        freeaddrinfo(result);
        return;
    }

    TAL_PR_DEBUG("SOCKET CONNECT SUCCESS!\n");


//     /*---------------------------------*/
//     /* Start of wolfSSL initialization and configuration */
//     /*---------------------------------*/
//     /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to initialize the library\n");
        // goto socket_cleanup;
        return;
    }
    TAL_PR_DEBUG("SSL INIT SUCCESS!\n");


//     /* Create and initialize WOLFSSL_CTX */
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

    if(wolfSSL_CTX_check_private_key(ctx) != WOLFSSL_SUCCESS)
    {
        TAL_PR_DEBUG(stderr, "Error wolfSSL_CTX_check_private_key\n");
        return 1;
    }
    TAL_PR_DEBUG("\t PV KEY CHECKED\n");


//     /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_buffer(ctx, CA_CERT, strlen(CA_CERT), WOLFSSL_FILETYPE_PEM))
         != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to load %s, please check the Cert. err: %d\n",CA_CERT, ret);
        return;
    }

    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, 0); 
    // wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL) ;

    
    TAL_PR_DEBUG("wolfSSL_CTX_load_verify_buffer SUCCESS!\n");

    // if(ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
    //                                      SHARED_KEY,
    //                                      strlen(SHARED_KEY),
    //                                      WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS)
    // {
    //     TAL_PR_DEBUG("ERROR: failed to load Client key %s, please check the Cert. err: %d\n",SHARED_KEY, ret);
    //     return;
    // }
    // TAL_PR_DEBUG("wolfSSL_CTX_use_PrivateKey_buffer SUCCESS!\n");



    //     /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        TAL_PR_DEBUG("ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        return;
    }
    TAL_PR_DEBUG("wolfSSL_new SUCCESS!\n");

    // Get the list of supported elliptic curves
    const char* curveList;
    // curveList = wolfSSL_get_curve_name(ssl);
    TAL_PR_DEBUG("\t\t Supported elliptic curves:\n");
    for(int i = 0; i< 30; i++)
    {
    curveList = wolfSSL_get_cipher_list(i);
        if (curveList == NULL) {
            TAL_PR_DEBUG("\t\t No elliptic curves supported.\n");
        } else {
            TAL_PR_DEBUG("%s\n", curveList);
        }
    }

    // char* ciphers;
    // ret = wolfSSL_get_ciphers(ciphers, (int)sizeof(ciphers));

    // if(ret == SSL_SUCCESS){
    //         TAL_PR_DEBUG("%s\n", ciphers);
    // }
    // else{
    //     TAL_PR_DEBUG("NO CIPHERS MATCHED\n");
    // }

    if(wolfSSL_CTX_UseSessionTicket(ctx) != WOLFSSL_SUCCESS)
    {
        TAL_PR_DEBUG("\t\tERROR: wolfSSL_CTX_UseSessionTicket.\n");
    }

// //     /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: Failed to set the file descriptor\n");
    }
    TAL_PR_DEBUG("attach wolfSSL to the socket SUCCESS!\n");
    
    // if(wolfSSL_UseSupportedCurve(ssl, WOLFSSL_ECC_SECP256R1) != WOLFSSL_SUCCESS)
    // {
    //     TAL_PR_DEBUG("ERROR : wolfSSL_UseSupportedCurve failed!\n");
    // }
    // TAL_PR_DEBUG("wolfSSL_UseSupportedCurve SUCCESS!\n");

    char* ciphers;
    ret = wolfSSL_get_ciphers(ciphers, (int)sizeof(ciphers));

    if(ret == WOLFSSL_SUCCESS){
            TAL_PR_DEBUG("%s\n", ciphers);
    }


// //     /* Connect to wolfSSL on the server side */
    // if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        TAL_PR_DEBUG("ERROR: failed to connect to wolfSSL error:%d\n", ret);
        return -1;
    }
    TAL_PR_DEBUG("Connect to wolfSSL on the server side SUCCESS!\n");


    /* Get a message for the server from stdin */
    TAL_PR_DEBUG("Message for server: ");
    // memset(buff, 0, sizeof(buff));
    // if (fgets(buff, sizeof(buff), stdin) == NULL) {
    //     TAL_PR_DEBUG("ERROR: failed to get message for server\n");
    //     ret = -1;
    // }
    // len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    // if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
    if ((ret = wolfSSL_write(ssl, GET_REQUEST, strlen(GET_REQUEST))) != len) {
        TAL_PR_DEBUG("ERROR: failed to write entire message\n");
        TAL_PR_DEBUG("%d bytes of %d bytes were sent", ret, (int) len);
    }

// //     /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        TAL_PR_DEBUG("ERROR: failed to read\n");
    }

// //     /* Print to stdout any data the server sends */
    TAL_PR_DEBUG("Server: %s\n", buff);

//     /* Bidirectional shutdown */
    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        TAL_PR_DEBUG("Shutdown not complete\n");
    }

    TAL_PR_DEBUG("Shutdown complete\n");

    ret = 0;

    /* Cleanup and return */
// cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
// ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
// socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
// end:
}