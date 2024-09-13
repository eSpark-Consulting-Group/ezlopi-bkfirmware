

#ifndef __EZLOPI_WEBSOCKET_CLINET_H__
#define __EZLOPI_WEBSOCKET_CLINET_H__

// #include "rtos_pub.h"
#include "bk_websocket_client.h"
#include "cJSON.h"
#include "httpc.h"

/*
    Details about websocekt protocol: https://datatracker.ietf.org/doc/html/rfc6455#page-27
*/

typedef enum
{
    OPERATION_CODE_CONTINUATION_FRAME_CODE = 0x0, /* denotes a continuation frame */
    OPERATION_CODE_TEXT_FRAME_CODE = 0x1,         /* denotes a text frame */
    OPERATION_CODE_BINARY_FRAME_CODE = 0x2,       /* denotes a binary frame */
    /* 0x3 - 0x7 are reserved for further non-control frames */
    OPERATION_CODE_CONNECTON_CLOSE_CODE = 0x8, /* denotes a connection close */
    OPERATION_CODE_PING_CODE = 0x9,            /* denotes a ping */
    OPERATION_CODE_PONG_CODE = 0xA,            /* denotes a pong */
    /* 0xB - 0xF  are reserved for further control frames */
    OPERATION_CODE_UNDEFINED,
} ezlopi_websocket_clinet_operation_code_t;

/* Data structure */

/*                         Base Framing Protocol
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    |     Extended payload length continued, if payload len == 127  |
    + - - - - - - - - - - - - - - - +-------------------------------+
    |                               |Masking-key, if MASK set to 1  |
    +-------------------------------+-------------------------------+
    | Masking-key (continued)       |          Payload Data         |
    +-------------------------------- - - - - - - - - - - - - - - - +
    :                     Payload Data continued ...                :
    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    |                     Payload Data continued ...                |
    +---------------------------------------------------------------+
    Resource : https://tools.ietf.org/html/rfc6455#page-27
*/
typedef struct
{
    uint8_t fin : 1;  /* Indicates that this is the final fragment in a message.  The first fragment MAY also be the final fragment. */
    uint8_t rsv1 : 1; /* MUST be 0 unless an extension is negotiated that defines meanings for non-zero values. (rsv1, rsv2, rsv3 are reserved bits) */
    uint8_t rsv2 : 1;
    uint8_t rsv3 : 1;
    uint8_t opcode : 4; /* Defines the interpretation of the "Payload data". Denoted by opcode_t above */

    uint8_t payload_len : 7; /* If 0 - 125, represents the length of the "Payload data". If 126  the following 2 bytes interpreted as a 16-bit unsigned integer are the payload length.
                                If 127, the following 8 bytes interpreted as a 64-bit unsigned integer. */

    uint8_t mask : 1; /* Defines whether the "Payload data" is masked.  If set to 1, a masking key is present in masking-key, and this is used to unmask
                         the "Payload data"*/
} frame_header_t;

typedef struct
{
    frame_header_t header;
    uint64_t payload_length;
    char masking_key[4];
    char *payload_data;
} s_ezlopi_websocket_frame_content_t;

typedef struct
{
    // beken_queue_t websocket_queue;
    http_session_t websocket_session;
    char *payload;
    uint64_t payload_length;
    ezlopi_websocket_clinet_operation_code_t opcode;
} s_ezlopi_websocket_t;

typedef struct s_ws_event_arg
{
    esp_websocket_client_handle_t client;
    void (*msg_upcall)(const char*, uint32_t);
    void (*connection_upcall)(bool connected);
} s_ws_event_arg_t;

int ezlopi_websocket_client_send_ping(http_session_t websocket_session);
int ezlopi_websocket_client_send_pong(http_session_t websocket_session);
int ezlopi_websocket_client_close(http_session_t websocket_session);
int ezlopi_websocekt_write_to_websocekt(http_session_t websocket_session, char *data, size_t data_len);
void ezlopi_websocket_register_upcalls(void (*msg_upcall)(const char*, uint32_t), void (*connection_upcall)(bool connected));
esp_websocket_client_handle_t ezlopi_websocket_client_init(cJSON* uri);
bool ezlopi_websocket_client_is_connected();
int ezlopi_websocket_client_send(char *data, uint32_t len);
#endif // __EZLOPI_WEBSOCKET_CLINET_H__
