
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "esp_err.h"
#include "esp_check.h"

#include "ezlopi_core_nvs.h"
#include "ezlopi_core_info.h"

#include "ezlopi_util_trace.h"

#include "ezlopi_core_processes.h"

#include "ezlopi_hal_uart.h"
#include "EZLOPI_USER_CONFIG.h"

static void ezlopi_uart_channel_task(void* args);
static ezlo_uart_channel_t get_available_channel();

// s_ezlopi_uart_object_handle_t ezlopi_uart_init(uint32_t baudrate, uint32_t tx, uint32_t rx, __uart_upcall upcall, void* arg)
// {
//     static QueueHandle_t ezlo_uart_channel_queue;
//     s_ezlopi_uart_object_handle_t uart_object_handle = (struct s_ezlopi_uart_object*)ezlopi_malloc(__FUNCTION__, sizeof(struct s_ezlopi_uart_object));
//     memset(uart_object_handle, 0, sizeof(struct s_ezlopi_uart_object));
//     uart_object_handle->arg = arg;
//     ezlo_uart_channel_t channel = get_available_channel();
//     if (NULL == upcall)
//     {
//         TRACE_E("NULL upcall found.");
//     }
//     else if (-1 != channel)
//     {
//         uart_config_t uart_config = {
//             .baud_rate = baudrate,
//             .data_width = DATA_WIDTH_8BIT,
//             .parity = BK_PARITY_NO,
//             .stop_bits = BK_STOP_BITS_1,
//             .flow_control = FLOW_CTRL_DISABLED,
//             // .rx_flow_ctrl_thresh = 0,
//             // .source_clk = UART_SCLK_APB,
//         };

//         if (EZLOPI_UART_CHANNEL_1 == channel)
//         {
//             ESP_ERROR_CHECK(uart_driver_install(channel, 256, 256, 32, &ezlo_uart_channel_queue, 0));
//             uart_object_handle->ezlopi_uart_queue_handle = ezlo_uart_channel_queue;
//         }
// #ifdef EZLOPI_UART_CHANNEL_2
//         else if (EZLOPI_UART_CHANNEL_2 == channel)
//         {
//             ESP_ERROR_CHECK(uart_driver_install(channel, 256, 256, 32, &ezlo_uart_channel_queue, 0));
//             uart_object_handle->ezlopi_uart_queue_handle = ezlo_uart_channel_queue;
//         }
// #endif
//         ESP_ERROR_CHECK(uart_param_config(channel, &uart_config));
//         ESP_ERROR_CHECK(uart_set_pin(channel, tx, rx, -1, -1));
//         TRACE_S("UART configured successfully.");
//         uart_object_handle->ezlopi_uart.channel = channel;
//         uart_object_handle->ezlopi_uart.baudrate = baudrate;
//         uart_object_handle->ezlopi_uart.tx = tx;
//         uart_object_handle->ezlopi_uart.rx = rx;
//         uart_object_handle->ezlopi_uart.enable = true;
//         uart_object_handle->upcall = upcall;

//         xTaskCreate(ezlopi_uart_channel_task, "EzpiUartChnTask", EZLOPI_HAL_UART_TASK_DEPTH, (void*)uart_object_handle, 13, &(uart_object_handle->taskHandle));
//         ezlopi_core_process_set_process_info(ENUM_EZLOPI_HAL_UART_TASK, &uart_object_handle->taskHandle, EZLOPI_HAL_UART_TASK_DEPTH);
//     }
//     else
//     {
//         TRACE_E("All channels are busy.");
//     }

//     return uart_object_handle;
// }

// static ezlo_uart_channel_t get_available_channel()
// {
//     TRACE_E("EZLOPI_UART_CHANNEL_MAX is : %d", EZLOPI_UART_CHANNEL_MAX);
//     for (ezlo_uart_channel_t channel = EZLOPI_UART_CHANNEL_1; channel < EZLOPI_UART_CHANNEL_MAX; channel++)
//     {
//         if (!uart_is_driver_installed(channel))
//         {
//             return channel;
//         }
//     }
//     return -1;
// }

// static void ezlopi_uart_channel_task(void* args)
// {
//     uart_event_t event;
//     uint8_t* buffer = NULL;

//     // s_ezlopi_uart_object_t *ezlopi_uart_object = (s_ezlopi_uart_object_t*)args;
//     s_ezlopi_uart_object_handle_t ezlopi_uart_object = (s_ezlopi_uart_object_handle_t)args;

//     while (1)
//     {
//         int data_len = 0;
//         // Start reveceiving UART events for first channel.
//         if (xQueueReceive(ezlopi_uart_object->ezlopi_uart_queue_handle, (void*)&event, portMAX_DELAY))
//         {
//             switch (event.type)
//             {
//             case UART_DATA:
//             {
//                 uint32_t bufferred_data_len = 0;
//                 uart_get_buffered_data_len(ezlopi_uart_object->ezlopi_uart.channel, &bufferred_data_len);
//                 if (bufferred_data_len)
//                 {
//                     buffer = ezlopi_malloc(__FUNCTION__, bufferred_data_len);
//                     if (buffer)
//                     {
//                         data_len = uart_read_bytes(ezlopi_uart_object->ezlopi_uart.channel, buffer, event.size, 100 / portTICK_PERIOD_MS);
//                     }
//                     else
//                     {
//                         uart_flush_input(ezlopi_uart_object->ezlopi_uart.channel);
//                     }
//                 }
//                 break;
//             }
//             case UART_BREAK:
//             case UART_BUFFER_FULL:
//             case UART_FIFO_OVF:
//             {
//                 TRACE_E("UART_FIFO_. event -> %d", event.type);
//                 uart_flush_input(ezlopi_uart_object->ezlopi_uart.channel);
//                 break;
//             }
//             default:
//             {
//                 TRACE_E("Default event encountered. %d", event.type);
//                 break;
//             }
//             }
//         }
//         ezlopi_uart_object->upcall(buffer, data_len, ezlopi_uart_object);
//         if (buffer)
//         {
//             ezlopi_free(__FUNCTION__, buffer);
//             buffer = NULL;
//         }
//         data_len = 0;
//     }
// }

// ezlo_uart_channel_t ezlopi_uart_get_channel(s_ezlopi_uart_object_handle_t ezlopi_uart_object_handle)
// {
//     return ezlopi_uart_object_handle->ezlopi_uart.channel;
// }

void EZPI_HAL_uart_init(void)
{
    uint32_t baud = EZPI_SERV_UART_BAUD_DEFAULT;
    uint32_t parity_val = EZPI_SERV_UART_PARITY_DEFAULT;
    uint32_t start_bits = EZPI_SERV_UART_START_BIT_DEFAULT;
    uint32_t stop_bits = EZPI_SERV_UART_STOP_BIT_DEFAULT;
    uint32_t frame_size = EZPI_SERV_UART_FRAME_SIZE_DEFAULT;
    uint32_t flow_control = EZPI_SERV_UART_FLOW_CTRL_DEFAULT;
    uart_data_width_t frame_size_val = EZPI_SERV_UART_FRAME_SIZE_DEFAULT;

    if (EZPI_CORE_nvs_read_baud(&baud))
    {
        EZPI_CORE_nvs_write_baud(EZPI_SERV_UART_BAUD_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    if (EZPI_CORE_nvs_read_parity(&parity_val))
    {
        EZPI_CORE_nvs_write_parity(EZPI_SERV_UART_PARITY_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    if (EZPI_CORE_nvs_read_start_bits(&start_bits))
    {
        EZPI_CORE_nvs_write_start_bits(EZPI_SERV_UART_START_BIT_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    if (EZPI_CORE_nvs_read_stop_bits(&stop_bits))
    {
        EZPI_CORE_nvs_write_stop_bits(EZPI_SERV_UART_STOP_BIT_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    if (EZPI_CORE_nvs_read_frame_size(&frame_size))
    {
        EZPI_CORE_nvs_write_frame_size(EZPI_SERV_UART_FRAME_SIZE_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    if (EZPI_CORE_nvs_read_flow_control(&flow_control))
    {
        EZPI_CORE_nvs_write_flow_control(EZPI_SERV_UART_FLOW_CTRL_DEFAULT);
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    frame_size_val = EZPI_CORE_info_get_frame_size(frame_size);

    char flw_ctrl_bffr[EZPI_UART_SERV_FLW_CTRL_STR_SIZE + 1];
    EZPI_CORE_info_get_flow_ctrl_to_name(flow_control, flw_ctrl_bffr);

    TRACE_I("Serial Configuration:");
    TRACE_I("  Baud Rate: %d", baud);
    TRACE_I("  Parity: %d", parity_val);
    TRACE_I("  Start Bits: %d", start_bits);
    TRACE_I("  Stop Bits: %d", stop_bits);
    TRACE_I("  Frame Size: %d", frame_size);
    TRACE_I("  Flow Control: %s", flw_ctrl_bffr);

    vTaskDelay(10 / portTICK_RATE_MS);

    {
        INT_T result = OPRT_OK;
        TAL_UART_CFG_T cfg = {0};
        cfg.base_cfg.baudrate = baud;
        cfg.base_cfg.databits = frame_size_val;
        cfg.base_cfg.stopbits = stop_bits;
        cfg.base_cfg.parity = parity_val;
        cfg.rx_buffer_size = 256;
        cfg.open_mode = O_BLOCK;
        result = tal_uart_init(TUYA_UART_NUM_0, &cfg);
        if (OPRT_OK != result) {
            TAL_PR_ERR("uart init failed", result);
        }
    }
}