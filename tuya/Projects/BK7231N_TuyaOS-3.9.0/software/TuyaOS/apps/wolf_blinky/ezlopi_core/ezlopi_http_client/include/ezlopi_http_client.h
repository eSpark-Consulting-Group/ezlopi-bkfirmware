
#ifndef __EZLOPI_HTTP_CLIENT_H__
#define __EZLOPI_HTTP_CLIENT_H__

#include "esp_http_client.h"

#define EZLOPI_HTTP_BUFFER_SIZE 1024

typedef struct s_rx_data
{
    char *ptr;
    int len;
    struct s_rx_data *next;

} s_rx_data_t;

int ezlopi_http_send_get_request(char *host_uri, char *received_data);
static esp_err_t ezlopi_http_event_handler(esp_http_client_event_t* evt);
int is_network_initialized();
#endif // __EZLOPI_HTTP_CLIENT_H__
