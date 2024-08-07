#ifndef ESP_PARTITION_H
#define ESP_PARTITION_H

#include "stdbool.h"

#include "esp_err.h"
#include "BkDriverFlash.h"

typedef enum {
    ESP_PARTITION_BOOTLOADER = 0,
    ESP_PARTITION_APPLICATION,
    ESP_PARTITION_OTA,
    ESP_PARTITION_RF_FIRMWARE,
    ESP_PARTITION_NET_PARAM,
    ESP_PARTITION_PROV_DATA,
    ESP_PARTITION_MAX,
} esp_partition_type_t;

typedef struct esp_partition_s {
    bk_partition_t type;
    uint32_t size;
    uint32_t start_addr;
    bool encrypted;
} esp_partition_t;

esp_err_t esp_partition_read(const esp_partition_t* partition,
                             size_t src_offset, void* dst, size_t size);

esp_err_t esp_partition_write(const esp_partition_t* partition,
                             size_t dst_offset, const void* src, size_t size);

esp_err_t esp_partition_erase_range(const esp_partition_t* partition,
                                    size_t offset, size_t size);

esp_partition_t* esp_partition_find_first(esp_partition_type_t type, const char* label);
#endif /* ESP_PARTITION_H */