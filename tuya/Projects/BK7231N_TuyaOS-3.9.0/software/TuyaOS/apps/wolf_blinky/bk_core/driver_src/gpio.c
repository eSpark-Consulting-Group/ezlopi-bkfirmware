#include "driver/gpio.h"

#include "esp_log.h"
#define TAG "driver/gpio"
#include "tkl_gpio.h"

#include <stdio.h>
#include <stdint.h>

// Function to extract pin numbers from the bitmask
static int get_first_pin_from_bitmask(uint64_t pin_bit_mask) {
    if (pin_bit_mask == 0) {
        return -1; // No pins are set
    }
    return __builtin_ctzll(pin_bit_mask); // Find the position of the first set bit
}

esp_err_t esp_gpio_config(const gpio_config_t *pGPIOConfig)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    TUYA_GPIO_BASE_CFG_T pin_config = {.mode = TUYA_GPIO_FLOATING, .direct = TUYA_GPIO_OUTPUT, .level = TUYA_GPIO_LEVEL_HIGH};
    tkl_gpio_init(get_first_pin_from_bitmask(pGPIOConfig->pin_bit_mask), &pin_config);
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    tkl_gpio_write(gpio_num, level);
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(gpio_num, &level);

    return (int)level;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    ESP_LOGE(TAG, "%s", __FUNCTION__);
    tkl_gpio_deinit(gpio_num);
    TUYA_GPIO_BASE_CFG_T pin_config = {0};
    switch (mode)
    {
    case GPIO_MODE_INPUT:
        pin_config.mode = TUYA_GPIO_HIGH_IMPEDANCE;
        pin_config.direct = TUYA_GPIO_INPUT;
        pin_config.level = TUYA_GPIO_LEVEL_LOW;
        break;
    case GPIO_MODE_OUTPUT:
        pin_config.mode = TUYA_GPIO_PUSH_PULL;
        pin_config.direct = TUYA_GPIO_OUTPUT;
        pin_config.level = TUYA_GPIO_LEVEL_LOW;
        break;
    case GPIO_MODE_OUTPUT_OD:
        pin_config.mode = TUYA_GPIO_OPENDRAIN;
        pin_config.direct = TUYA_GPIO_OUTPUT;
        pin_config.level = TUYA_GPIO_LEVEL_LOW;
        break;
    case GPIO_MODE_INPUT_OUTPUT_OD:
        break;
    case GPIO_MODE_INPUT_OUTPUT:
        break;
    
    case GPIO_MODE_DISABLE:
    default:
        break;
    }
    tkl_gpio_init(gpio_num, &pin_config);
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
    tkl_gpio_irq_enable(gpio_num);
    return ESP_OK;
}