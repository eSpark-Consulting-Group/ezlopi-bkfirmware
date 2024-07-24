
#include "ezlopi_http_client.h"
#include "tal_log.h"
#include "httpc.h"
#include "cert.h"
// #include "str_pub.h"
// #include "uni_log.h"
// #include "string.h"

/* mbed tls api headers */
#include "mbedtls/platform.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/x509_crt.h"

#define SERVER_PORT "7000"
#define SERVER_NAME "cloud.ezlo.com"
#define GET_REQUEST "GET /getserver?json=true HTTP/1.0\r\n\r\n"

static http_session_t https_handle;

static int copy_data(char *data, int read_size, s_rx_data_t *my_data)
{
    int ret = 0;
    
    if (my_data)
    {
        my_data->ptr = (char *)os_malloc(read_size + 1);
        if (my_data->ptr)
        {
            os_snprintf(my_data->ptr, read_size + 1, "%s", data);
            my_data->len = read_size;
            my_data->next = NULL;
        }
        else
        {
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }
    return ret;
}

static int set_https_response_data_to_ll(char *data, int read_size, s_rx_data_t **my_data)
{
    int ret = 0;

    if (!(*my_data))
    {
        *my_data = (s_rx_data_t *)os_malloc(sizeof(s_rx_data_t));
        copy_data(data, read_size, *my_data);
    }
    else
    {
        s_rx_data_t *current_prt = *my_data;
        while (NULL != current_prt->next)
        {
            current_prt = current_prt->next;
        }
        current_prt->next = (s_rx_data_t *)os_malloc(sizeof(s_rx_data_t));
        copy_data(data, read_size, current_prt->next);
    }

    return ret;
}

static int get_https_received_data_total_len(s_rx_data_t *my_data, int *received_len)
{
    int ret = 0;

    if (my_data)
    {
        *received_len = 0;
        s_rx_data_t *current_ptr = my_data;
        while (current_ptr)
        {
            *received_len += current_ptr->len;
            current_ptr = current_ptr->next;
        }
    }
    else
    {
        ret = 1;
    }

    return ret;
}

static void https_free_rx_data(s_rx_data_t *my_data)
{
    if (my_data)
    {
        if (my_data->next)
        {
            https_free_rx_data(my_data->next);
        }
        if (my_data->ptr)
        {
            free(my_data->ptr);
        }
    }
}

static int https_send_request(char *host_uri)
{
    int ret = 0;
    
    http_req_t https_request = {
        .type = HTTP_GET,
        .resource = host_uri,
        .version = HTTP_VER_1_1,
        .redirect_cnt = REDIRECT_CNT_MAX,
        .content = NULL,
        .content_len = 0,
        .add_head_cb = NULL,
        .add_head_data = NULL,
        .download_size = 0,
        .download_offset = 0,
        .p_custom_content_ctx = NULL,
    };
    // It is mandatory to call before sending the request.
    http_prepare_req(https_handle, &https_request, 0);
    if (WM_SUCCESS == http_send_request(https_handle, &https_request, 1))
    {
        TAL_PR_DEBUG("Request sent successfully.");
        ret = 0;
    }
    else
    {
        TAL_PR_ERR("Failed to send request.");
        ret = 1;
    }
    return ret;
}

static int https_read_response(char ** p_received_data)
{
    int ret = 0;

    http_resp_t *http_resp = NULL;
    http_get_response_hdr(https_handle, &http_resp);
    // Process only after the status code is 200/OK.
    if (200 == http_resp->status_code)
    {
        char *receive_buffer = (char *)os_malloc(EZLOPI_HTTP_BUFFER_SIZE);
        s_rx_data_t *my_data = NULL;
        if (receive_buffer)
        {
            int read_size = 0;
            do
            {
                memset(receive_buffer, 0, sizeof(EZLOPI_HTTP_BUFFER_SIZE));
                read_size = http_read_content(https_handle, receive_buffer, EZLOPI_HTTP_BUFFER_SIZE);
                if (read_size != 0)
                {
                    set_https_response_data_to_ll(receive_buffer, read_size, &my_data);
                }
                // TAL_PR_ERR("In do while");

            } while (read_size != 0);
            int receive_data_total_len = 0;
            get_https_received_data_total_len(my_data, &receive_data_total_len);
            if (receive_data_total_len != 0)
            {
                *p_received_data = (char *)os_malloc(receive_data_total_len + 1);
                char *received_data = *p_received_data;
                if (received_data)
                {
                    os_memset(received_data, 0, receive_data_total_len + 1);
                    s_rx_data_t *current_ptr = my_data;
                    while (current_ptr)
                    {
                        strcat(received_data, current_ptr->ptr);
                        current_ptr = current_ptr->next;
                    }
                }
                else
                {
                    TAL_PR_ERR("Error combining received data.");
                    ret = 1;
                }
            }
            else
            {
                TAL_PR_ERR("No data received.");
                ret = 1;
            }
        }
        else
        {
            TAL_PR_ERR("Unable to allocate memory for receiver's buffer.");
            ret = 1;
        }
        free(receive_buffer);
        https_free_rx_data(my_data);
    }
    else
    {
        TAL_PR_ERR("No status code 200 received.(code: %d)", http_resp->status_code);
        ret = 1;
    }

    return ret;
}

// int ezlopi_http_send_get_request(char *host_uri, char *received_data)
// {
//     int ret = 0;

//     // TLS_ENABLE is sent as a flag for creating HTTPS client.
//     // retry_cnt is set to 0 for letting SDK determing the socket creation retry count, also can be set here.
//     int http_ret = http_open_session_with_config(&https_handle, host_uri, TLS_ENABLE, 0);
//     if (WM_SUCCESS == http_ret)
//     {
//         TAL_PR_DEBUG("HTTP session created successfully");
//         // if (0 == https_send_request(host_uri))
//         // {
//         //     if (0 == https_read_response(received_data))
//         //     {
//         //         TAL_PR_DEBUG("Data reading successful.(data: %s)", received_data);
//         //     }
//         //     else
//         //     {
//         //         TAL_PR_ERR("Something went wrong while receiving response form the server.");
//         //         ret = 1;
//         //     }
//         // }
//         // else
//         // {
//         //     TAL_PR_ERR("Error while sending data to the server.");
//         //     ret = 1;
//         // }
//         // // Since http session has been created, any kind of error encountered or any data is received, the session has to be closed.
//         // http_close_session(&https_handle);
//     }
//     else
//     {
//         TAL_PR_ERR("Error opening HTTP session.(code: %d)", http_ret);
//         ret = 1;
//     }

//     return ret;
// }

int ezlopi_http_send_get_request(char *host_uri, char *received_data)
{
    int ret = 0;
    
    // Configure the TLS parameters
    tuya_tls_config_t tls_config;
    memset(&tls_config, 0, sizeof(tls_config));
    tls_config.mode = TUYA_TLS_MUTUAL_CERT_MODE;  // Example mode, adjust as needed
    tls_config.hostname = host_uri;
    tls_config.port = 7000;  // HTTPS port

    // Set CA certificate (assuming CA_CERT is defined as per your previous definitions)
    tls_config.ca_cert = CA_CERT;
    tls_config.ca_cert_size = strlen(CA_CERT);

    // Set timeout and other parameters as needed
    tls_config.timeout = 10;  // Example timeout
    tls_config.verify = true;  // Enable certificate verification

    // If you have client certificate and key, set them as follows:
    tls_config.client_cert = SHARED_KEY;
    tls_config.client_cert_size = strlen(SHARED_KEY);
    tls_config.client_pkey = SSL_PRIVATE_KEY;
    tls_config.client_pkey_size = strlen(SSL_PRIVATE_KEY);

    // Open HTTP session with Tuya TLS configuration
    int http_ret = http_open_session_with_config(&https_handle, host_uri, &tls_config, TLS_ENABLE, 0);
    // int http_ret = http_open_session(&https_handle, host_uri, NULL, 0);
    if (WM_SUCCESS == http_ret)
    {
        TAL_PR_DEBUG("HTTP session created successfully");
        // Proceed with sending request and reading response as per your implementation
        if (0 == https_send_request(host_uri))
        {
            if (0 == https_read_response(&received_data))
            {
                TAL_PR_DEBUG("Data reading successful.(data: %s)", received_data);
            }
            else
            {
                TAL_PR_ERR("Something went wrong while receiving response form the server.");
                ret = 1;
            }
        }
        else
        {
            TAL_PR_ERR("Error while sending data to the server.");
            ret = 1;
        }
        // Since http session has been created, any kind of error encountered or any data is received, the session has to be closed.
        http_close_session(&https_handle);
    }
    else
    {
        TAL_PR_ERR("Error opening HTTP session.(code: %d)", http_ret);
        ret = 1;
    }

    return ret;
}


int is_network_initialized() {
    int ret = system("ping -c 1 google.com > /dev/null 2>&1");
    TAL_PR_ERR("RETURN:%d\r\n",ret);
    if (ret == 0) {
        TAL_PR_DEBUG("Network is connected.\n");
        return 1; // Network is connected
    } else {
        TAL_PR_DEBUG("Network is not connected.\n");
        return 0; // Network is not connected
    }
}

static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
    ((void) level);

    // mbedtls_fprintf((FILE *) ctx, "%s:%04d: %s", file, line, str);
    // fflush((FILE *) ctx);
}

int ezlopi_mbedtls_https_client(char *host_uri, char *received_data)
{
    
    int ret = 1, len;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_net_context server_fd;
    uint32_t flags;
    unsigned char buf[1024];
    const char *pers = "ssl_client1";

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_pk_context pkey;

// #if defined(MBEDTLS_DEBUG_C)
//     mbedtls_debug_set_threshold(DEBUG_LEVEL);
// #endif

    /*
     * 0. Initialize the RNG and the session data
     */
    // mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    // mbedtls_x509_crt_init(&cacert);
    // mbedtls_x509_crt_init(&clicert);
    // mbedtls_pk_init(&pkey);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_printf("\n  . Seeding the random number generator...");
    fflush(stdout);


    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
        return -1;
    }

    mbedtls_printf(" ok\n");

    /*
     * 1. Load the trusted CA
     */
    mbedtls_printf("  . Loading the CA root certificate ...");
    fflush(stdout);

    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *) CA_CERT,
                                 strlen(CA_CERT) + 1);
    if (ret < 0) {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%04x\n\n", -ret);
        goto exit;
    }

    mbedtls_printf(" ok (%d skipped)\n", ret);

    psa_crypto_init();

    /*
     * 2. Load the client certificate and private key
     */
    mbedtls_printf("  . Loading the client certificate and private key ...");
    fflush(stdout);

    ret = mbedtls_x509_crt_parse(&clicert, (const unsigned char *) SHARED_KEY,
                                 strlen(SHARED_KEY) + 1);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", (unsigned int) -ret);
        goto exit;
    }

    ret = mbedtls_pk_parse_key(&pkey, (const unsigned char *) SSL_PRIVATE_KEY,
                               strlen(SSL_PRIVATE_KEY) + 1, NULL, 0, NULL, 0);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_pk_parse_key returned -0x%x\n\n", (unsigned int) -ret);
        goto exit;
    }

    mbedtls_printf(" ok\n");

    /*
     * 3. Start the connection
     */
    mbedtls_printf("  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT);
    fflush(stdout);

    if ((ret = mbedtls_net_connect(&server_fd, SERVER_NAME,
                                   SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_printf(" ok\n");

    /*
     * 4. Setup stuff
     */
    mbedtls_printf("  . Setting up the SSL/TLS structure...");
    fflush(stdout);

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

    ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /*
     * 5. Handshake
     */
    mbedtls_printf("  . Performing the SSL/TLS handshake...");
    fflush(stdout);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int) -ret);
            goto exit;
        }
    }

    mbedtls_printf(" ok\n");

    /*
     * 6. Verify the server certificate
     */
    mbedtls_printf("  . Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
        char vrfy_buf[512];

        mbedtls_printf(" failed\n");

        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

        mbedtls_printf("%s\n", vrfy_buf);
    } else
        mbedtls_printf(" ok\n");

    /*
     * 7. Write the GET request
     */
    mbedtls_printf("  > Write to server:");
    fflush(stdout);

    len = snprintf((char *) buf, sizeof(buf) - 1, GET_REQUEST);

    while ((ret = mbedtls_ssl_write(&ssl, buf, len)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
            goto exit;
        }
    }

    len = ret;
    mbedtls_printf(" %d bytes written\n\n%s", len, (char *) buf);

    /*
     * 8. Read the HTTP response
     */
    mbedtls_printf("  < Read from server:");
    fflush(stdout);

    do {
        len = sizeof(buf) - 1;
        memset(buf, 0, sizeof(buf));
        ret = mbedtls_ssl_read(&ssl, buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            break;

        if (ret < 0) {
            mbedtls_printf("failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
            break;
        }

        if (ret == 0) {
            mbedtls_printf("\n\nEOF\n\n");
            break;
        }

        len = ret;
        mbedtls_printf(" %d bytes read\n\n%s", len, (char *) buf);
    } while (1);

    mbedtls_ssl_close_notify(&ssl);

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    status = psa_crypto_deinit();
    if (status != PSA_SUCCESS) {
        mbedtls_fprintf(stderr, "Failed to deinitialize PSA Crypto implementation: %d\n",
                        (int) status);
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

#ifdef MBEDTLS_ERROR_C
    if (exit_code != MBEDTLS_EXIT_SUCCESS) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif

    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_x509_crt_free(&clicert);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    mbedtls_exit(exit_code);
    
    return 0;
}