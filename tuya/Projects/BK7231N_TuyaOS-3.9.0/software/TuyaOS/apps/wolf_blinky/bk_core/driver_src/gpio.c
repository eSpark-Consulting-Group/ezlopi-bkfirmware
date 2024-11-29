#include "driver/gpio.h"

#include "esp_log.h"
#define TAG "driver/gpio"


esp_err_t esp_gpio_config(const gpio_config_t *pGPIOConfig)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;   
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;   
}

esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;   
}

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_install_isr_service(int intr_alloc_flags)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

void gpio_uninstall_isr_service(void)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}

esp_err_t gpio_intr_enable(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    return ESP_OK;
}