// #include "stdbool.h"
// #include "mem_pub.h"
// #include "str_pub.h"
// #include "base64_enc_dec.h"
// #include "uni_random.h"
// #include "ezlopi_core_websocket.h"
// #include "cert.h"
// #include "cJSON.h"
// #include "httpc.h"

// #define WEBSOCKET_TASK_STACK_SIZE 1024
// #define WEBSOCKET_RECEIVE_BUFFER_SIZE 1024 * sizeof(char)
// #define WEBSOCKET_CLIENT_KEY_LEN 24

// static char *generate_random_16_byte_base64_value()
// {
//     char *random_16_byte_value = (char *)os_malloc(20 * sizeof(char));
//     os_memset(random_16_byte_value, 0, (20 * sizeof(char)));
//     uni_random_string(random_16_byte_value, 16);
//     return random_16_byte_value; // remember to free it by callee.
// }

// static int ezlopi_websocket_client_add_upgrade_headers(s_ezlopi_websocket_t *websocket, http_req_t *wss_upgrade_request)
// {
//     int ret = 0;

//     char *random_16_byte_value = generate_random_16_byte_base64_value();
//     char random_16_byte_base64_value[200];
//     os_memset(random_16_byte_base64_value, 0, 200);
//     bintob64(random_16_byte_base64_value, random_16_byte_value, strlen(random_16_byte_value));

//     http_add_header(websocket->websocket_session, wss_upgrade_request, "Connection", "Upgrade");
//     http_add_header(websocket->websocket_session, wss_upgrade_request, "Upgrade", "websocket");
//     http_add_header(websocket->websocket_session, wss_upgrade_request, "Sec-WebSocket-Key", random_16_byte_base64_value);
//     http_add_header(websocket->websocket_session, wss_upgrade_request, "Sec-WebSocket-Protocol", "chat");
//     http_add_header(websocket->websocket_session, wss_upgrade_request, "Sec-WebSocket-Version", "13");

//     return ret;
// }

// static int ezlopi_websocket_client_send_upgrade_request(s_ezlopi_websocket_t *websocket, char *uri)
// {
//     int ret = 0;

//     http_req_t wss_upgrade_request = {
//         .type = HTTP_GET,
//         .resource = uri,
//         .version = HTTP_VER_1_1,
//         .redirect_cnt = REDIRECT_CNT_MAX,
//         .content = NULL,
//         .content_len = 0,
//         .add_head_cb = NULL,
//         .add_head_data = NULL,
//         .download_size = 0,
//         .download_offset = 0,
//         .p_custom_content_ctx = NULL,
//     };
//     int error = http_prepare_req(websocket->websocket_session, &wss_upgrade_request, 0);
//     if (WM_SUCCESS == error)
//     {
//         PR_DEBUG("Upgrade request prepared.");
//         ezlopi_websocket_client_add_upgrade_headers(websocket, &wss_upgrade_request);
//         error = http_send_request(websocket->websocket_session, &wss_upgrade_request, 1);
//         if (WM_SUCCESS == error)
//         {
//             PR_DEBUG("Request sent successfully.");
//             ret = 0;
//         }
//         else
//         {
//             PR_ERR("Failed to send request.(code: %d)", error);
//             ret = 1;
//         }
//     }
//     else
//     {
//         PR_ERR("Error preparing request. (code: %d)", error);
//     }

//     return ret;
// }

// static int ezlopi_websocket_client_read_upgrade_response(s_ezlopi_websocket_t *websocket)
// {
//     int ret = 0;

//     http_resp_t *wss_upgrade_response = NULL;
//     http_get_response_hdr(websocket->websocket_session, &wss_upgrade_response);
//     if (101 == wss_upgrade_response->status_code)
//     {
//         TAL_PR_DEBUG("Upgraded to websocket successfully.");
//         http_header_pair_t *websocket_upgrade_handshake = NULL;
//         int count = 0;
//         int error = http_get_response_hdr_all(websocket->websocket_session, websocket_upgrade_handshake, &count);
//         if (WM_SUCCESS == error)
//         {
//             for (int i = 0; i < count; i++)
//             {
//                 TAL_PR_DEBUG("%s: %s", websocket_upgrade_handshake[i].name, websocket_upgrade_handshake[i].value);
//             }
//         }
//         else
//         {
//             TAL_PR_ERR("Error getting the header.(code: %d)", error);
//             ret = 1;
//         }
//     }
//     else
//     {
//         TAL_PR_ERR("Error upgrading to websocket.(code: %d)", wss_upgrade_response->status_code);
//     }

//     return ret;
// }

// int ezlopi_websocket_client_initialize_websocket_client(char *uri, s_ezlopi_websocket_t *websocket)
// {
//     int ret = 0;

//     int wss_ret = http_open_session(&websocket->websocket_session, uri, TLS_ENABLE, 0);
//     if (WM_SUCCESS == wss_ret)
//     {
//         TAL_PR_DEBUG("HTTP session created successfully");
//         if (0 == ezlopi_websocket_client_send_upgrade_request(websocket, uri))
//         {
//             TAL_PR_DEBUG("Upgrade request sent successfully.");
//             if (0 == ezlopi_websocket_client_read_upgrade_response(websocket))
//             {
//                 TAL_PR_DEBUG("Websocekt created successfully.");
//                 rtos_create_thread(NULL, 5, "ezlopi_websocekt_client_read_frames", ezlopi_websocekt_client_read_frames, WEBSOCKET_TASK_STACK_SIZE, websocket);
//             }
//             else
//             {
//                 TAL_PR_ERR("Something went wrong during websocket creation. Error in response.");
//                 http_close_session(websocket->websocket_session);
//                 ret = 1;
//             }
//         }
//         else
//         {
//             TAL_PR_ERR("Something went wrong while sending upgrade request.");
//             http_close_session(websocket->websocket_session);
//             ret = 1;
//         }
//     }
//     else
//     {
//         TAL_PR_ERR("Error initiating WSS session.(code: %d)", wss_ret);
//         ret = 0;
//     }

//     return ret;
// }