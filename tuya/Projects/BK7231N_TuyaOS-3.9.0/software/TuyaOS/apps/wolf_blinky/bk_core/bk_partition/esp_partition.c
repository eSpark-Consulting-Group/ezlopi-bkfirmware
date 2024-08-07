#include "esp_partition.h"
#include "ezlopi_util_trace.h"
#include "assert.h"

esp_err_t esp_partition_read(const esp_partition_t *partition,
                             size_t src_offset, void *dst, size_t size)
{
    int ret = ESP_OK;
    assert(partition != NULL);
    if (src_offset > partition->size) {
        TRACE_E("ESP_ERR_INVALID_ARG");
        ret = ESP_ERR_INVALID_ARG;
        return ret;
    }
    if (size > partition->size - src_offset) {
        TRACE_E("ESP_ERR_INVALID_SIZE");
        ret = ESP_ERR_INVALID_SIZE;
        return ret;
    }

    if (!partition->encrypted) {
        ret = bk_flash_read(partition->type, src_offset, dst, size);
    }

    return ret;
}

esp_err_t esp_partition_write(const esp_partition_t *partition,
                              size_t dst_offset, const void *src, size_t size)
{
    assert(partition != NULL);
    // if (partition->readonly) {
    //     return ESP_ERR_NOT_ALLOWED;
    // }
    if (dst_offset > partition->size) {
        TRACE_E("ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }
    if (size > partition->size - dst_offset) {
        TRACE_E("ESP_ERR_INVALID_SIZE");
        return ESP_ERR_INVALID_SIZE;
    }
    if (!partition->encrypted) {
        // bk_flash_erase(partition->type, dst_offset, size);
        return bk_flash_write(partition->type, dst_offset, src, size);
    }
}

esp_partition_t* esp_partition_find_first(esp_partition_type_t type, const char* label)
{
    bk_logic_partition_t *info = bk_flash_get_info((bk_partition_t)type);
    esp_partition_t* __partition_ctx = (esp_partition_t*)malloc(sizeof(esp_partition_t));
    __partition_ctx->encrypted = false;
    __partition_ctx->type = type;
    __partition_ctx->size = info->partition_length;
    __partition_ctx->start_addr = info->partition_start_addr;
    hal_flash_init();
    return __partition_ctx;
}

esp_err_t esp_partition_erase_range(const esp_partition_t *partition,
                                    size_t offset, size_t size)
{
    assert(partition != NULL);
    // if (partition->readonly) {
    //     return ESP_ERR_NOT_ALLOWED;
    // }
    if (offset > partition->size) {
        return ESP_ERR_INVALID_ARG;
    }
    if (size > partition->size - offset) {
        return ESP_ERR_INVALID_SIZE;
    }

    return bk_flash_erase(partition->type, offset, size);
}