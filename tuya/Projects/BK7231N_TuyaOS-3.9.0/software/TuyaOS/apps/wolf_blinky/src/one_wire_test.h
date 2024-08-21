#ifndef ONE_WIRE_H
#define ONE_WIRE_H

#include <stdint.h>
#include <stdbool.h>

// Define the GPIO pin for One-Wire communication
#define ONE_WIRE_GPIO_PIN 4  // Example GPIO pin number

// Function prototypes for GPIO control (to be implemented for your platform)
void gpio_set_direction_output(uint8_t pin);
void gpio_set_direction_input(uint8_t pin);
void gpio_set_low(uint8_t pin);
void gpio_set_high(uint8_t pin);
bool gpio_read(uint8_t pin);
void delay_us(unsigned int us);  // Microsecond delay

// One-Wire function prototypes
void one_wire_init();
void one_wire_reset();
void one_wire_write_bit(uint8_t bit);
uint8_t one_wire_read_bit();
void one_wire_write_byte(uint8_t byte);
uint8_t one_wire_read_byte();
void delay_test();
void dht11_init(); 
    

#endif  // ONE_WIRE_H
