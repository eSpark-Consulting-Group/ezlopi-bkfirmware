#ifndef _EZLOPI_HAL_GPIO_H_
#define _EZLOPI_HAL_GPIO_H_

#include <stdio.h>
#include "tkl_gpio.h"

typedef TUYA_GPIO_NUM_E e_ezlopi_gpio_num_t;
typedef TUYA_GPIO_DRCT_E e_ezlopi_gpio_mode_t;
typedef TUYA_GPIO_MODE_E e_ezlopi_gpio_pull_mode_t;
typedef TUYA_GPIO_IRQ_E e_ezlopi_gpio_interrupt_t;

#define GPIO_IS_VALID_OUTPUT_GPIO(x) true //fixme
#define GPIO_IS_VALID_GPIO(x) true //fixme
#define GPIO_MODE_OUTPUT TUYA_GPIO_OUTPUT
#define GPIO_MODE_INPUT TUYA_GPIO_INPUT

typedef enum e_ezlopi_gpio_value
{
    EZLOPI_GPIO_LOW = 0,
    EZLOPI_GPIO_HIGH,
    EZLOPI_GPIO_VALUE_MAX
} e_ezlopi_gpio_value_t;

typedef enum e_ezlopi_gpio_logic_invert
{
    EZLOPI_GPIO_LOGIC_NONINVERTED = 0,
    EZLOPI_GPIO_LOGIC_INVERTED,
    EZLOPI_GPIO_LOGIC_MAX
} e_ezlopi_gpio_logic_invert_t;

typedef struct s_ezlopi_gpio
{
    bool enable;
    e_ezlopi_gpio_num_t gpio_num;
    e_ezlopi_gpio_mode_t mode;
    e_ezlopi_gpio_value_t value;
    e_ezlopi_gpio_pull_mode_t pull;
    e_ezlopi_gpio_logic_invert_t invert;
    e_ezlopi_gpio_interrupt_t interrupt;
} s_ezlopi_gpio_t;

typedef struct s_ezlopi_gpios
{
    s_ezlopi_gpio_t gpio_in;
    s_ezlopi_gpio_t gpio_out;
} s_ezlopi_gpios_t;

// int ezlopi_gpio_init(s_ezlopi_device_properties_t *properties);

#endif // _EZLOPI_HAL_GPIO_H_
