#include "esp_partition.h"
#include "ezlopi_util_trace.h"
#include "assert.h"

esp_err_t esp_partition_read(const esp_partition_t *partition,
                             size_t src_offset, void *dst, size_t size)
{
    esp_err_t ret = ESP_OK;
    assert(partition != NULL);
    if (src_offset > partition->size) {
        TRACE_E("ESP_ERR_INVALID_ARG");
        ret = ESP_ERR_INVALID_ARG;
        return ret;
    }
    if (size > partition->size - src_offset) {
        TRACE_E("ESP_ERR_INVALID_SIZE part size %d , offset %d, size %d", partition->size, src_offset, size);
        ret = ESP_ERR_INVALID_SIZE;
        return ret;
    }

    if (!partition->encrypted) {
        ret = bk_flash_read(partition->type, src_offset, dst, size);
    }
    // TRACE_W("%s returned %x", __FUNCTION__, ret);
    return ret;
}

esp_err_t esp_partition_write(const esp_partition_t *partition,
                              size_t dst_offset, const void *src, size_t size)
{
    assert(partition != NULL);
    esp_err_t ret = ESP_OK;
    // if (partition->readonly) {
    //     return ESP_ERR_NOT_ALLOWED;
    // }
    if (dst_offset > partition->size) {
        TRACE_E("ESP_ERR_INVALID_ARG");
        ret = ESP_ERR_INVALID_ARG;
        return ret;
    }
    if (size > partition->size - dst_offset) {
        // TRACE_E("ESP_ERR_INVALID_SIZE");
        TRACE_E("ESP_ERR_INVALID_SIZE part size %d , offset %d, size %d", partition->size, dst_offset, size);
        ret = ESP_ERR_INVALID_SIZE;
        return ret;
    }
    if (!partition->encrypted) {
        // bk_flash_erase(partition->type, dst_offset, size);
        ret = bk_flash_write(partition->type, dst_offset, src, size);
    }
    // TRACE_W("%s returned %x", __FUNCTION__, ret);
    return ret;
}

esp_partition_t* esp_partition_find_first(esp_partition_type_t type, const char* label)
{
    hal_flash_init();
    bk_logic_partition_t *info = bk_flash_get_info((bk_partition_t)type);
    esp_partition_t* __partition_ctx = (esp_partition_t*)malloc(sizeof(esp_partition_t));
    __partition_ctx->encrypted = false;
    __partition_ctx->type = type;
    __partition_ctx->size = info->partition_length;
    __partition_ctx->start_addr = info->partition_start_addr;
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
        TRACE_E("ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }
    if (size > partition->size - offset) {
        TRACE_E("ESP_ERR_INVALID_SIZE part size %d , offset %d, size %d", partition->size, offset, size);
        return ESP_ERR_INVALID_SIZE;
    }

    return bk_flash_erase(partition->type, offset, size);
}