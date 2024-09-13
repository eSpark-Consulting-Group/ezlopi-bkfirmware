#include <stdlib.h>
#include "tal_log.h"
#include "uni_log.h"
#include "tkl_gpio.h"
#include "tal_uart.h"
#include "tal_wifi.h"
#include "tkl_output.h"
#include "tal_thread.h"
#include "tal_system.h"
#include "tal_memory.h"

#ifdef ENABLE_WIFI_SERVICE
#include "tuya_iot_wifi_api.h"
#else
#include "tuya_iot_base_api.h"
#endif
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
#include "lwip_init.h"
#endif

#include "ezlopi_core_wifi.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_service_broadcast.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_ezlopi.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_log.h"
#include "ezlopi_service_loop.h"
#include "ezlopi_core_nvs.h"
#include "ezlopi_core_event_group.h"

#include "FreeRTOS.h"
#include "task.h"

#include "esp_partition.h"

#define PID  "tvjs8eysp0imads8"
#define LED_PIN TUYA_GPIO_NUM_26


/* Function Prototypes */

STATIC VOID __free_heap_task(PVOID_T args);
uint32_t __ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
void flash_read_write_test(void);
VOID example_kv();

/* Thread Handles */

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE bk_rcv_thread = NULL;
STATIC THREAD_HANDLE free_heap_thrd = NULL;

#if 0
void test_nvs_string_operations() {
    const char* key = "test_key";
    const char* value_to_write = "test_value";

    // Write the string to NVS
    esp_err_t write_result = nvs_set_str(NULL, key, value_to_write);
    if (write_result == ESP_OK) {
        TRACE_I("String written successfully.\n");
    } else {
        TRACE_I("Failed to write string. Error code: %d\n", write_result);
        return;
    }

    // Read the string from NVS
    size_t required_length = 0;
    esp_err_t read_result = nvs_get_str(NULL, key, NULL, &required_length);
    if (read_result == ESP_OK) {
        char* read_value = (char*)malloc(required_length);
        if (read_value == NULL) {
            TRACE_I("Failed to allocate memory for reading string.\n");
            return;
        }

        read_result = nvs_get_str(NULL, key, read_value, &required_length);
        if (read_result == ESP_OK) {
            TRACE_I("String read successfully: %s\n", read_value);
        } else {
            TRACE_I("Failed to read string. Error code: %d\n", read_result);
        }

        free(read_value);
    } else if (read_result == ESP_ERR_NVS_NOT_FOUND) {
        TRACE_I("Key not found.\n");
    } else {
        TRACE_I("Failed to read string length. Error code: %d\n", read_result);
    }
}
#endif

#if 0
void test_nvs_apis() {
    esp_err_t err;
    
    // Test int8_t
    int8_t i8_write = -128, i8_read = 0;
    err = nvs_set_i8(NULL, "i8_key", i8_write);
    if (err == ESP_OK) {
        err = nvs_get_i8(NULL, "i8_key", &i8_read);
        TRACE_W("int8_t: written = %d, read = %d, status = %s\n", i8_write, i8_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test uint8_t
    uint8_t u8_write = 255, u8_read = 0;
    err = nvs_set_u8(NULL, "u8_key", u8_write);
    if (err == ESP_OK) {
        err = nvs_get_u8(NULL, "u8_key", &u8_read);
        TRACE_W("uint8_t: written = %u, read = %u, status = %s\n", u8_write, u8_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test int16_t
    int16_t i16_write = -32768, i16_read = 0;
    err = nvs_set_i16(NULL, "i16_key", i16_write);
    if (err == ESP_OK) {
        err = nvs_get_i16(NULL, "i16_key", &i16_read);
        TRACE_W("int16_t: written = %d, read = %d, status = %s\n", i16_write, i16_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test uint16_t
    uint16_t u16_write = 65535, u16_read = 0;
    err = nvs_set_u16(NULL, "u16_key", u16_write);
    if (err == ESP_OK) {
        err = nvs_get_u16(NULL, "u16_key", &u16_read);
        TRACE_W("uint16_t: written = %u, read = %u, status = %s\n", u16_write, u16_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test int32_t
    int32_t i32_write = -2147483648, i32_read = 0;
    err = nvs_set_i32(NULL, "i32_key", i32_write);
    if (err == ESP_OK) {
        err = nvs_get_i32(NULL, "i32_key", &i32_read);
        TRACE_W("int32_t: written = %d, read = %d, status = %s\n", i32_write, i32_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test uint32_t
    uint32_t u32_write = 4294967295, u32_read = 0;
    err = nvs_set_u32(NULL, "u32_key", u32_write);
    if (err == ESP_OK) {
        err = nvs_get_u32(NULL, "u32_key", &u32_read);
        TRACE_W("uint32_t: written = %u, read = %u, status = %s\n", u32_write, u32_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    #if 0
    // Test int64_t
    int64_t i64_write = -9223372036854775807LL, i64_read = 0;
    err = nvs_set_i64(NULL, "i64_key", i64_write);
    if (err == ESP_OK) {
        err = nvs_get_i64(NULL, "i64_key", &i64_read);
        TRACE_W("int64_t: written = %lld, read = %lld, status = %s\n", i64_write, i64_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test uint64_t
    uint64_t u64_write = 18446744073709551615ULL, u64_read = 0;
    err = nvs_set_u64(NULL, "u64_key", u64_write);
    if (err == ESP_OK) {
        err = nvs_get_u64(NULL, "u64_key", &u64_read);
        TRACE_W("uint64_t: written = %llu, read = %llu, status = %s\n", u64_write, u64_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }
    #endif

    // Test string
    const char *str_write = "Hello, NVS!";
    char str_read[20] = {0};
    size_t str_len = sizeof(str_read);
    err = nvs_set_str(NULL, "str_key", str_write);
    if (err == ESP_OK) {
        err = nvs_get_str(NULL, "str_key", str_read, &str_len);
        TRACE_W("string: written = %s, read = %s, status = %s\n", str_write, str_read, err == ESP_OK ? "SUCCESS" : "FAIL");
    }

    // Test blob
    uint8_t blob_write[5] = {1, 2, 3, 4, 5};
    uint8_t blob_read[5] = {0};
    size_t blob_len = sizeof(blob_read);
    err = nvs_set_blob(NULL, "blob_key", blob_write, sizeof(blob_write));
    if (err == ESP_OK) {
        err = nvs_get_blob(NULL, "blob_key", blob_read, &blob_len);
        TRACE_W("blob: written = {1, 2, 3, 4, 5}, read = {%u, %u, %u, %u, %u}, status = %s\n",
               blob_read[0], blob_read[1], blob_read[2], blob_read[3], blob_read[4],
               err == ESP_OK ? "SUCCESS" : "FAIL");
    }
}
#endif

void dummy_world()
{
    TRACE_W("Dummy World");
}
STATIC VOID user_main(VOID_T)
{
    OPERATE_RET rt = OPRT_OK;

    /* LwIP Initialization */
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    TUYA_LwIP_Init();
#endif

    TUYA_CALL_ERR_LOG(tuya_iot_init("./"));

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
    THREAD_CFG_T thread_cfg = 
    {
        .thrdname = "free_heap",
        .stackDepth = 2048 * 3,
        .priority = THREAD_PRIO_0,
    };
    if (NULL == free_heap_thrd) 
    {
        tal_thread_create_and_start(&free_heap_thrd, NULL, NULL, __free_heap_task, NULL, &thread_cfg);
    }

    // TaskHandle_t free_heap_task_handle = NULL;
    // xTaskCreate(__free_heap_task, "freeheap-service", 2048, NULL, 2, &free_heap_task_handle);

    ezlopi_core_set_log_upcalls();

    ezlopi_init();

    ezlopi_service_broadcast_init();

    ezlopi_service_web_provisioning_init();

    ezlopi_nvs_set_boot_count(ezlopi_system_info_get_boot_count() + 1);

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
    THREAD_CFG_T thrd_param = {4096*4, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
STATIC VOID __free_heap_task(PVOID_T args)
{
    int free_heap = 0;
    float free_heap_kb;
    // tal_system_sleep(30000);

    // ezlopi_service_loop_init();
    uint32_t last_tick_count = 0;
    for (;;) {
        free_heap = tal_system_get_free_heap_size();
        free_heap_kb = free_heap/1024.0;
        // TAL_PR_NOTICE("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        TRACE_I("----------------------------------------------");
        TRACE_W("free heap: %d B       %.4f KB", free_heap, free_heap_kb);
        TRACE_I("----------------------------------------------");
        if(free_heap <= 5*1024)
        {
            tal_system_reset();
        }
         
        uint32_t tick_count_ms = xTaskGetTickCount() / portTICK_PERIOD_MS;
        // TRACE_I("TICK COUNT = %d, TICK DIFF = %d", tick_count_ms, tick_count_ms - last_tick_count);
        last_tick_count = tick_count_ms;
        if(ezlopi_event_group_wait_for_event(EZLOPI_EVENT_WS_CONNECT, 0 ,false) && !ezlopi_event_group_wait_for_event(EZLOPI_EVENT_LOOP_INIT, 0 ,false))
        {
            ezlopi_service_loop_init();
            ezlopi_event_group_set_event(EZLOPI_EVENT_LOOP_INIT);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    tal_thread_delete(free_heap_thrd);
}

#if 0
uint32_t ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type);
void flash_read_write_test()
{
    // Initialize the flash hardware
    
    hal_flash_init();
    int data_offset = __ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA);
    
    // Prepare the input buffer and its length
    char* inBuffer = " Flash Hello World World Write !";
    int inBufferLen = strlen(inBuffer);

    // Ensure the output buffer is large enough
    char outBuffer[256] = {0};

    TRACE_S("Before -----> %s", outBuffer);

    // // Erase the flash sector before writing
    // if (bk_flash_erase(BK_PARTITION_PROV_DATA, data_offset, inBufferLen) != 0) {
    //     TRACE_E("Flash Erase Failed");
    //     return;
    // }

    // Write to flash
    // if (bk_flash_write(BK_PARTITION_PROV_DATA, data_offset, inBuffer, inBufferLen) == 0) {
    //     TRACE_I("Flash Write Success");
    // } else {
    //     TRACE_E("Flash Write Failed");
    //     return;
    // }

    // Read from flash
    if (bk_flash_read(BK_PARTITION_PROV_DATA, EZLOPI_FINFO_OFFSET_HUB_DATA + EZLOPI_FINFO_LEN_WIFI_SSID, outBuffer, 96) == 0) {
        TRACE_I("Flash Read Success");
    } else {
        TRACE_E("Flash Read Failed");
        return;
    }

    TRACE_S("After -----> %s", outBuffer);
}
#endif

#if 0
void flash_read_write_test()
{
    char* inBuffer = "Hello ESP BK Partition!";
    char outBuffer[256] = "Garbage Value Garbage Value !";
    int inBufferLen = strlen(inBuffer);
    
    esp_partition_t* partition_ctx_v3 = esp_partition_find_first(ESP_PARTITION_PROV_DATA, EZLOPI_FACTORY_INFO_V3_PARTITION_NAME);

    if(NULL == partition_ctx_v3)
    {
        TRACE_E("Partition CTX is NULL");
    }
    TRACE_W("Before Write Output Buffer = %s", outBuffer);
    esp_partition_write(partition_ctx_v3, 0x0, inBuffer, inBufferLen);
    esp_partition_read(partition_ctx_v3, 0x0, outBuffer, inBufferLen);
    TRACE_W("After Write Output Buffer = %s", outBuffer);
    
}
#endif

#if 0
uint32_t __ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type)
{
    if (partition_type == E_EZLOPI_FACTORY_INFO_CONN_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_CONN_DATA);
    }
    else if (partition_type == E_EZLOPI_FACTORY_INFO_HUB_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_HUB_DATA);
    }
    else
    {
        return 0;
    }
}
#endif

// #define TEST_KV_DB
#if defined TEST_KV_DB
/**
* @brief kv_common task
*
* @param[in] param:Task parameters
* @return none
*/
#define KEY_NAME "my_key"

VOID example_kv()
{
    OPERATE_RET rt = OPRT_OK;
    INT_T i;
    BYTE_T  *read_buf = NULL;
    UINT_T read_len;
    BOOL_T is_exist = FALSE;

    TAL_PR_NOTICE("------ kv common example start ------");

    /* Write "my_key" value to kv database */
    BYTE_T write_buf[] = {0x00, 0x01, 0x02, 0x03};

    TUYA_CALL_ERR_GOTO(wd_common_write(KEY_NAME, write_buf, CNTSOF(write_buf)), __EXIT);

    /* Output the values written to the KV database */
    TAL_PR_NOTICE("kv flash write data:");
    for (i=0; i<CNTSOF(write_buf); i++) {
        TAL_PR_DEBUG_RAW("0x%02x ", write_buf[i]);
    }
    TAL_PR_DEBUG_RAW("\r\n\r\n");

    /* Check whether "my_key" exists in the kv database */
    TUYA_CALL_ERR_GOTO(wd_common_exist(KEY_NAME, &is_exist), __EXIT);
    if (FALSE == is_exist) {
        TAL_PR_NOTICE("my_key is not exist in the kv database");
        goto __EXIT;
    }

    /* read "my_key" value from kv database */
    TUYA_CALL_ERR_GOTO(wd_common_read(KEY_NAME, &read_buf, &read_len), __EXIT);

    TAL_PR_NOTICE("read len %d, read data:", read_len);
    for ( i = 0; i < read_len; i++) {
        TAL_PR_DEBUG_RAW("0x%02x ", read_buf[i]);
    }
    TAL_PR_DEBUG_RAW("\r\n\r\n");

__EXIT:
    /*free data*/
    if (NULL != read_buf) {
        TUYA_CALL_ERR_LOG(wd_common_free_data(read_buf));
        read_buf = NULL;
    }

    /*delete key*/
    rt = wd_common_delete(KEY_NAME);
    if(OPRT_OK != rt) {
        TAL_PR_ERR("err<%d>,delete fail!", rt);
    } else {
        TAL_PR_NOTICE("my_key is deleted in kv database");
    }

    return;
}
#endif //TEST_KV_DB