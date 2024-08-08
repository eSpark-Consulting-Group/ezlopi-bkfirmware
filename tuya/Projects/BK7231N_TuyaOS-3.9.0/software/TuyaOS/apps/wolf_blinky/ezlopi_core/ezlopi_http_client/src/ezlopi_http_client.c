
#include "esp_http_client.h"
#include "ezlopi_http_client.h"
#include "ezlopi_util_trace.h"
#include "httpc.h"

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
        TRACE_D("Request sent successfully.");
        ret = 0;
    }
    else
    {
        TRACE_E("Failed to send request.");
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
                // TRACE_E("In do while");

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
                    TRACE_E("Error combining received data.");
                    ret = 1;
                }
            }
            else
            {
                TRACE_E("No data received.");
                ret = 1;
            }
        }
        else
        {
            TRACE_E("Unable to allocate memory for receiver's buffer.");
            ret = 1;
        }
        free(receive_buffer);
        https_free_rx_data(my_data);
    }
    else
    {
        TRACE_E("No status code 200 received.(code: %d)", http_resp->status_code);
        ret = 1;
    }

    return ret;
}

static esp_err_t ezlopi_http_event_handler(esp_http_client_event_t* evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
    {
        TRACE_D("HTTP_EVENT_ERROR");
        break;
    }
    case HTTP_EVENT_ON_CONNECTED:
    {
        TRACE_D("HTTP_EVENT_ON_CONNECTED");
        break;
    }
    case HTTP_EVENT_HEADER_SENT:
    {
        TRACE_D("HTTP_EVENT_HEADER_SENT");
        break;
    }
    case HTTP_EVENT_ON_HEADER:
    {
        TRACE_D("HTTP_EVENT_ON_HEADER, \nheaders = %s ", evt->data);
        break;
    }
    case HTTP_EVENT_ON_DATA:
    {
        TRACE_D("HTTP_EVENT_ON_DATA, len=%d, data: \n%.*s", evt->data_len, evt->data_len, (char*)evt->data);
        // if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (evt->user_data)
            {
                char* tmp_data = (char*)malloc(evt->data_len + 1);
                if (NULL != tmp_data)
                {
                    memcpy(tmp_data, evt->data, evt->data_len);
                    tmp_data[evt->data_len] = '\0';

                    s_rx_data_t* my_data = (s_rx_data_t*)evt->user_data;
                    if (my_data->len || my_data->total_len || my_data->ptr)
                    {
                        s_rx_data_t* cur_dh = my_data;
                        while (cur_dh->next)
                        {
                            cur_dh = cur_dh->next;
                        }

                        cur_dh->next = (s_rx_data_t*)malloc(sizeof(s_rx_data_t));
                        if (cur_dh->next)
                        {
                            cur_dh->next->len = evt->data_len;
                            cur_dh->next->ptr = (char*)tmp_data;
                            cur_dh->next->total_len = 0;
                            my_data->total_len += evt->data_len;
                            cur_dh->next->next = NULL;
                        }
                    }
                    else
                    {
                        my_data->len = evt->data_len;
                        my_data->total_len = evt->data_len;
                        my_data->ptr = (char*)tmp_data;
                        my_data->next = NULL;
                    }
                }
            }
        }

        break;
    }
    case HTTP_EVENT_ON_FINISH:
    {
        TRACE_D("HTTP_EVENT_ON_FINISH");
        break;
    }
    case HTTP_EVENT_DISCONNECTED:
    {
        TRACE_D("HTTP_EVENT_DISCONNECTED");
        break;
    }
    }
    return ESP_OK;
}


static void ezlopi_http_free_rx_data(s_rx_data_t* rx_data)
{
    if (rx_data)
    {
        if (rx_data->next)
        {
            ezlopi_http_free_rx_data(rx_data->next);
        }

        if (rx_data->ptr)
        {
            free( rx_data->ptr);
        }
        free(rx_data);
    }
}

s_ezlopi_http_data_t* ezlopi_http_get_request(const char* cloud_url, const char* private_key, const char* shared_key, const char* ca_certificate)
{
    char* ret = NULL;
    int status_code = 0;
    s_rx_data_t* my_data = (s_rx_data_t*)malloc(sizeof(s_rx_data_t));
    s_ezlopi_http_data_t* http_get_data = NULL;
    if ((NULL != my_data))
    {
        memset(my_data, 0, sizeof(s_rx_data_t));

        esp_http_client_config_t config = {
            .url = cloud_url,
            .cert_pem = ca_certificate,
            .client_cert_pem = shared_key,
            .client_key_pem = private_key,
            .event_handler = NULL,
            .port = 7000,
            .event_handler = ezlopi_http_event_handler,
            .user_data = (void*)(my_data), // my_data will be filled in 'ezlopi_http_event_handler'
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (NULL != client)
        {   
            esp_http_client_set_method(client, HTTP_METHOD_GET);
            esp_err_t err = esp_http_client_perform(client);
            status_code = esp_http_client_get_status_code(client);
            if (my_data->total_len && (err == ESP_OK))
            {
                ret = (char*)malloc(my_data->total_len + 1);
                if (ret)
                {
                    s_rx_data_t* cur_d = my_data;
                    memset(ret, 0, my_data->total_len + 1);

                    while (cur_d)
                    {
                        strcat(ret, cur_d->ptr);
                        TRACE_D("%.*s", cur_d->len, cur_d->ptr);
                        cur_d = cur_d->next;
                    }

                    http_get_data = (s_ezlopi_http_data_t*)malloc(sizeof(s_ezlopi_http_data_t));
                    if (http_get_data)
                    {
                        memset(http_get_data, 0, sizeof(s_ezlopi_http_data_t));
                        http_get_data->response = ret;
                        http_get_data->status_code = status_code;
                    }
                }
            }
            else
            {
                TRACE_E("Error perform http request %d", (int)err);
            }

            ezlopi_http_free_rx_data(my_data);
            esp_http_client_cleanup(client);
        }
    }
    return http_get_data;
}


int is_network_initialized() {
    int ret = system("ping -c 1 google.com > /dev/null 2>&1");
    TRACE_E("RETURN:%d\r\n",ret);
    if (ret == 0) {
        TRACE_D("Network is connected.\n");
        return 1; // Network is connected
    } else {
        TRACE_D("Network is not connected.\n");
        return 0; // Network is not connected
    }
}