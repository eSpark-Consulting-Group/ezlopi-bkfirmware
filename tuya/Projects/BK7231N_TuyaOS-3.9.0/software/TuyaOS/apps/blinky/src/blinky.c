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

#ifdef ENABLE_WIFI_SERVICE
#include "tuya_iot_wifi_api.h"
#else
#include "tuya_iot_base_api.h"
#endif
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
#include "lwip_init.h"
#endif

// #include "tuya_app_config.h"

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_24

THREAD_HANDLE ty_app_thread = NULL;

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
    
    tal_log_set_manage_attr(TAL_LOG_LEVEL_TRACE);

    ezlopi_wifi_initialize();
    ezlopi_wifi_connect_from_id_bin();

    while (!ezlopi_wifi_is_connected()) {
        TAL_PR_ERR("Not Connected");
        tal_system_sleep(1000);
    }
    TAL_PR_ERR("Wifi Connected");

    // is_network_initialized();
    // ezlopi_http_send_get_request("https://192.168.18.95/esp", NULL);
    ezlopi_http_send_get_request("https://cloud.ezlo.com:7000/getserver?json=true/", NULL);
    // ezlopi_http_send_get_request("http://httpbin.org/", NULL);
    // ezlopi_http_send_get_request("https://a3.tuyacn.com/", NULL);
    // ezlopi_http_send_get_request("https://google.com/", NULL);
   
    while(1) {  
        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_HIGH);
        // tal_uart_write(TUYA_UART_NUM_0, turnOn, sizeof(turnOn));
        tal_system_sleep(blink_interval);

        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_LOW);
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
    cJSON_free(NULL);
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}