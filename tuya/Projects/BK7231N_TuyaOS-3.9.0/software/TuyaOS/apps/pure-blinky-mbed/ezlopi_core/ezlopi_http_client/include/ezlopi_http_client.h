
#ifndef __EZLOPI_HTTP_CLIENT_H__
#define __EZLOPI_HTTP_CLIENT_H__

#define EZLOPI_HTTP_BUFFER_SIZE 1024

typedef struct s_rx_data
{
    char *ptr;
    int len;
    struct s_rx_data *next;

} s_rx_data_t;

int ezlopi_http_send_get_request(char *host_uri, char *received_data);

int is_network_initialized();
#endif // __EZLOPI_HTTP_CLIENT_H__
