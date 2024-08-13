#include "esp_nvs.h"

esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key) {
    OPERATE_RET ret = wd_common_delete(key);
    if (ret == OPRT_OK) {
        return ESP_OK;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    // Tuya's API does not require a commit step, so we just return ESP_OK.
    return ESP_OK;
}

esp_err_t nvs_set_i8(nvs_handle_t handle, const char* key, int8_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(int8_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(uint8_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(int16_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(uint16_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(int32_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(uint32_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

#if 0
esp_err_t nvs_set_i64(nvs_handle_t handle, const char* key, int64_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(int64_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
    return wd_common_write(key, (BYTE_T *)&value, sizeof(uint64_t)) == OPRT_OK ? ESP_OK : ESP_FAIL;
}
#endif

esp_err_t nvs_set_str(nvs_handle_t handle, const char* key, const char* value) {
    UINT_T len = strlen(value) + 1; // Include the null-terminator
    return wd_common_write(key, (BYTE_T *)value, len) == OPRT_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length) {
    return wd_common_write(key, (BYTE_T *)value, length) == OPRT_OK ? ESP_OK : ESP_FAIL;
}


esp_err_t nvs_get_i8(nvs_handle_t handle, const char* key, int8_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(int8_t)) {
        *out_value = *((int8_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}


esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(uint8_t)) {
        *out_value = *((uint8_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}


esp_err_t nvs_get_i16(nvs_handle_t handle, const char* key, int16_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(int16_t)) {
        *out_value = *((int16_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(uint16_t)) {
        *out_value = *((uint16_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(int32_t)) {
        *out_value = *((int32_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(uint32_t)) {
        *out_value = *((uint32_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

#if 0
esp_err_t nvs_get_i64(nvs_handle_t handle, const char* key, int64_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(int64_t)) {
        *out_value = *((int64_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_get_u64(nvs_handle_t handle, const char* key, uint64_t* out_value) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK && len == sizeof(uint64_t)) {
        *out_value = *((uint64_t *)data);
        wd_common_free_data(data);
        return ESP_OK;
    } else if (ret == OPRT_OK) {
        wd_common_free_data(data);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    return ESP_ERR_NVS_NOT_FOUND;
}
#endif

esp_err_t nvs_get_str(nvs_handle_t handle, const char* key, char* out_value, size_t* length) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK) {
        if (out_value == NULL) {
            *length = len; // Set the required length
            wd_common_free_data(data);
            return ESP_OK;
        } else if (*length >= len) {
            strncpy(out_value, (char *)data, len);
            *length = len; // Set the actual length
            wd_common_free_data(data);
            return ESP_OK;
        } else {
            wd_common_free_data(data);
            return ESP_ERR_NVS_INVALID_LENGTH;
        }
    }
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length) {
    BYTE_T *data = NULL;
    UINT_T len = 0;
    OPERATE_RET ret = wd_common_read(key, &data, &len);
    if (ret == OPRT_OK) {
        if (out_value == NULL) {
            *length = len; // Set the required length
            wd_common_free_data(data);
            return ESP_OK;
        } else if (*length >= len) {
            memcpy(out_value, data, len);
            *length = len; // Set the actual length
            wd_common_free_data(data);
            return ESP_OK;
        } else {
            wd_common_free_data(data);
            return ESP_ERR_NVS_INVALID_LENGTH;
        }
    }
    return ESP_ERR_NVS_NOT_FOUND;
}
