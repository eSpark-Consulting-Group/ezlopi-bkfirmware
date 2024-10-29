/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"

#include "tal_thread.h"
#include "tal_system.h"
#include "tal_log.h"

// #include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt.h"
#include "esp_err.h"

#if CONFIG_BT_ENABLED

// typedef enum bt_task_status_e{

// }bt_task_status_t;

// static esp_gatts_cb_t       __esp_gatts_callback;
// static esp_gap_ble_cb_t     __esp_gap_callback;
void bt_task()
{
    while(1)
    {
        TAL_PR_DEBUG("bt_task running ......");
        tal_system_sleep(10000);
    }
}

esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg)
{
    esp_err_t err;
    uint32_t btdm_cfg_mask = 0;

    THREAD_HANDLE ezlopi_service_timer_task_handle = NULL;
    THREAD_CFG_T thread_cfg = {cfg->controller_task_stack_size, cfg->controller_task_prio,"bt_task"};
    tal_thread_create_and_start(&ezlopi_service_timer_task_handle, NULL, NULL, bt_task, NULL, &thread_cfg);

    return err;
}

// esp_err_t esp_ble_gatts_register_callback(esp_gatts_cb_t callback)
// {
//     esp_err_t ret = ESP_OK;
//     __esp_gatts_callback = callback;
//     return ret;
// }

// esp_err_t esp_ble_gap_register_callback(esp_gap_ble_cb_t callback)
// {
//     esp_err_t ret = ESP_OK;
//     __esp_gap_callback = callback;
//     return ret;
// }

#endif /*  CONFIG_BT_ENABLED */