
#include "ezlopi_http_client.h"
#include "tal_log.h"
#include "httpc.h"
#include "cert.h"

#include "http_client_interface.h"
#include "mbedtls/ssl.h"

// #include "str_pub.h"
// #include "uni_log.h"
// #include "string.h"

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

/* 
typedef INT_T(*tuya_tls_send_cb)(VOID *p_custom_net_ctx, CONST BYTE_T *buf, SIZE_T len);
typedef INT_T(*tuya_tls_recv_cb)(VOID *p_custom_net_ctx, BYTE_T *buf, SIZE_T len);
typedef VOID (*tuya_tls_event_cb)(TY_TLS_EVENT_E event, VOID *p_args);

// */
// VOID event_callback(TY_TLS_EVENT_E arg1, VOID* arg2)
// {
//      TAL_PR_DEBUG("Event Callback called!\n");
// }

// INT_T send_callback(VOID* arg1, CONST BYTE_T *arg2, SIZE_T arg3)
// {
//      TAL_PR_DEBUG("Send Callback called!\n");
// }

// INT_T recv_callback(VOID* arg1, BYTE_T *arg2, SIZE_T arg3)
// {
//      TAL_PR_DEBUG("Recv Callback called!\n");
// }

int ezlopi_http_send_get_request(char *host_uri, char *received_data)
{
    int ret = 0;
    
    // Configure the TLS parameters
    tuya_tls_config_t tls_config;
    // memset(&tls_config, 0, sizeof(tls_config));
    tls_config.mode = TUYA_TLS_MUTUAL_CERT_MODE;  // Example mode, adjust as needed
    // tls_config.mode = TUYA_TLS_PSK_MODE;  // Example mode, adjust as needed
    // tls_config.mode = TUYA_TLS_AWS_FFS_CERT_MODE;  // Example mode, adjust as needed
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

    /* Setup callbacks */
    // tls_config.f_send = send_callback;
    // tls_config.f_recv = recv_callback;
    // tls_config.exception_cb = event_callback;

    // Open HTTP session with Tuya TLS configuration
    int http_ret = http_open_session_with_config(&https_handle, host_uri, &tls_config, TLS_ENABLE, 0);
    // int http_ret = http_open_session(&https_handle, host_uri, 0, 0);
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

/*
typedef struct http_client_request {
    const char*           host;
    uint16_t              port;
    const char*           path;
    const uint8_t*        cacert;
    size_t                cacert_len;
    const char*           method;
    http_client_header_t* headers;
    uint8_t               headers_count;
    const uint8_t*        body;
    size_t                body_length;
    uint32_t              timeout_ms;
} http_client_request_t;
*/
// OPERATE_RET tuya_tls_connect(tuya_tls_hander p_tls_handler, IN CHAR_T *hostname, IN INT_T port_num,
//                              IN INT_T socket_fd, IN INT_T overtime_s)
// {
// TAL_PR_ERR("new tls connect");
// }
http_client_header_t headers[] = {  
    {.key = "Content-Type", .value = "application/json"}
};

// int ezlopi_http_send_get_request(char *host_uri, char *received_data)
// {
//     http_client_status_t http_status;

//     http_client_request_t get_request = {
//         .host = "cloud.ezlo.com",
//         .port = 7000,
//         .path = "/getserver?json=true",
//         .cacert = CA_CERT,
//         .cacert_len = strlen(CA_CERT),
//         .method = "GET",
//         .headers = headers,
//         .headers_count = sizeof(headers)/sizeof(http_client_header_t),
//         // .headers = NULL,
//         // .headers_count = 0,
//         .body = NULL,
//         .body_length = 0,
//         .timeout_ms = 5000,
//     };

//     http_client_response_t response_rcv = {0};
//     // tuya_tls_connect(NULL, NULL, NULL, NULL, NULL);
//     http_status = http_client_request(&get_request, &response_rcv);
//         // mbedtls_ssl_write(NULL, NULL, 0);
//         if (HTTP_CLIENT_SUCCESS != http_status) {
//         TAL_PR_ERR("http_request_send error:%d", http_status);
//         return OPRT_LINK_CORE_HTTP_CLIENT_SEND_ERROR;
//     }
// }



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

