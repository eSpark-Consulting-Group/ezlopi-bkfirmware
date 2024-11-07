/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESP_BT_H__
#define __ESP_BT_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sdkconfig.h"
// #include "esp_task.h"

// #include "nimble/nimble_npl.h"
// #include "esp_bt_cfg.h"

#ifdef CONFIG_BT_LE_HCI_INTERFACE_USE_UART
#include "driver/uart.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth mode for controller enable/disable
 */

typedef enum {
    ESP_BT_MODE_IDLE       = 0x00,   /*!< Bluetooth is not running */
    ESP_BT_MODE_BLE        = 0x01,   /*!< Run BLE mode */
    ESP_BT_MODE_CLASSIC_BT = 0x02,   /*!< Run Classic BT mode */
    ESP_BT_MODE_BTDM       = 0x03,   /*!< Run dual mode */
} esp_bt_mode_t;

/**
 * @brief Bluetooth controller enable/disable/initialised/de-initialised status
 */

typedef enum {
    ESP_BT_CONTROLLER_STATUS_IDLE = 0,   /*!< Controller is in idle state */
    ESP_BT_CONTROLLER_STATUS_INITED,     /*!< Controller is in initialising state */
    ESP_BT_CONTROLLER_STATUS_ENABLED,    /*!< Controller is in enabled state */
    ESP_BT_CONTROLLER_STATUS_NUM,        /*!< Controller is in disabled state */
} esp_bt_controller_status_t;

/**
 * @brief BLE tx power type
 *        ESP_BLE_PWR_TYPE_CONN_HDL0-8: for each connection, and only be set after connection completed.
 *                                      when disconnect, the correspond TX power is not effected.
 *        ESP_BLE_PWR_TYPE_ADV : for advertising/scan response.
 *        ESP_BLE_PWR_TYPE_SCAN : for scan.
 *        ESP_BLE_PWR_TYPE_DEFAULT : if each connection's TX power is not set, it will use this default value.
 *                                   if neither in scan mode nor in adv mode, it will use this default value.
 *        If none of power type is set, system will use ESP_PWR_LVL_P3 as default for ADV/SCAN/CONN0-9.
 */
typedef enum {
    ESP_BLE_PWR_TYPE_CONN_HDL0  = 0,            /*!< For connection handle 0 */
    ESP_BLE_PWR_TYPE_CONN_HDL1  = 1,            /*!< For connection handle 1 */
    ESP_BLE_PWR_TYPE_CONN_HDL2  = 2,            /*!< For connection handle 2 */
    ESP_BLE_PWR_TYPE_CONN_HDL3  = 3,            /*!< For connection handle 3 */
    ESP_BLE_PWR_TYPE_CONN_HDL4  = 4,            /*!< For connection handle 4 */
    ESP_BLE_PWR_TYPE_CONN_HDL5  = 5,            /*!< For connection handle 5 */
    ESP_BLE_PWR_TYPE_CONN_HDL6  = 6,            /*!< For connection handle 6 */
    ESP_BLE_PWR_TYPE_CONN_HDL7  = 7,            /*!< For connection handle 7 */
    ESP_BLE_PWR_TYPE_CONN_HDL8  = 8,            /*!< For connection handle 8 */
    ESP_BLE_PWR_TYPE_ADV        = 9,            /*!< For advertising */
    ESP_BLE_PWR_TYPE_SCAN       = 10,           /*!< For scan */
    ESP_BLE_PWR_TYPE_DEFAULT    = 11,           /*!< For default, if not set other, it will use default value */
    ESP_BLE_PWR_TYPE_NUM        = 12,           /*!< TYPE numbers */
} esp_ble_power_type_t;

/**
 * @brief Bluetooth TX power level(index), it's just a index corresponding to power(dbm).
 */
typedef enum {
    ESP_PWR_LVL_N27 = 0,              /*!< Corresponding to -27dbm */
    ESP_PWR_LVL_N24 = 1,              /*!< Corresponding to -24dbm */
    ESP_PWR_LVL_N21 = 2,              /*!< Corresponding to -21dbm */
    ESP_PWR_LVL_N18 = 3,              /*!< Corresponding to -18dbm */
    ESP_PWR_LVL_N15 = 4,              /*!< Corresponding to -15dbm */
    ESP_PWR_LVL_N12 = 5,              /*!< Corresponding to -12dbm */
    ESP_PWR_LVL_N9  = 6,              /*!< Corresponding to  -9dbm */
    ESP_PWR_LVL_N6  = 7,              /*!< Corresponding to  -6dbm */
    ESP_PWR_LVL_N3  = 8,              /*!< Corresponding to  -3dbm */
    ESP_PWR_LVL_N0  = 9,              /*!< Corresponding to   0dbm */
    ESP_PWR_LVL_P3  = 10,             /*!< Corresponding to  +3dbm */
    ESP_PWR_LVL_P6  = 11,             /*!< Corresponding to  +6dbm */
    ESP_PWR_LVL_P9  = 12,             /*!< Corresponding to  +9dbm */
    ESP_PWR_LVL_P12 = 13,             /*!< Corresponding to  +12dbm */
    ESP_PWR_LVL_P15 = 14,             /*!< Corresponding to  +15dbm */
    ESP_PWR_LVL_P18 = 15,             /*!< Corresponding to  +18dbm */
    ESP_PWR_LVL_INVALID = 0xFF,         /*!< Indicates an invalid value */
} esp_power_level_t;

typedef enum {
    ESP_BLE_ENHANCED_PWR_TYPE_DEFAULT = 0,
    ESP_BLE_ENHANCED_PWR_TYPE_ADV,
    ESP_BLE_ENHANCED_PWR_TYPE_SCAN,
    ESP_BLE_ENHANCED_PWR_TYPE_INIT,
    ESP_BLE_ENHANCED_PWR_TYPE_CONN,
    ESP_BLE_ENHANCED_PWR_TYPE_MAX,
} esp_ble_enhanced_power_type_t;

typedef struct {
    uint8_t type;
    uint8_t val[6];
} esp_ble_addr_t;

#define CONFIG_VERSION  0x20220824
#define CONFIG_MAGIC    0x5A5AA5A5

/**
 * @brief Controller config options, depend on config mask.
 *        Config mask indicate which functions enabled, this means
 *        some options or parameters of some functions enabled by config mask.
 */

typedef struct {
    uint16_t ble_acl_buf_size;
    uint16_t ble_acl_buf_count;
    uint16_t ble_hci_evt_buf_size;
    uint16_t ble_multi_adv_instances;
    uint16_t ble_ext_adv_max_size;
    uint16_t controller_task_stack_size;
    uint8_t controller_task_prio;
    uint8_t controller_run_cpu;
} esp_bt_controller_config_t;


#define BT_CONTROLLER_INIT_CONFIG_DEFAULT() {                                              \
    .controller_task_stack_size = 6*1024                                                   \
}

esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg);
void* esp_bt_get_event_stack_ptr();

#ifdef __cplusplus
}
#endif

#endif /* __ESP_BT_H__ */