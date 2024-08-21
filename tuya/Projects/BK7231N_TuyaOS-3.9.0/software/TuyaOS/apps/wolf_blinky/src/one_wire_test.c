#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>  // For usleep() to implement delays

#include "one_wire_test.h"  // Rename to dht11_test.h if needed
#include "tal_log.h"
#include "tal_system.h"
#include "tkl_gpio.h"

#define GPIO_PIN 26  // Example GPIO pin number for DHT11

// Placeholder functions for GPIO control (unchanged from original code)
void gpio_set_direction_output(uint8_t pin);
void gpio_set_direction_input(uint8_t pin);
void gpio_set_low(uint8_t pin);
void gpio_set_high(uint8_t pin);
bool gpio_read(uint8_t pin);
void delay_us(unsigned int us);  // Microsecond delay

#define TICK_US 4
#define US_DELAY(us) \
do { \
    volatile UINT_T i = us * TICK_US; \
    while (i--); \
} while(0)

// DHT11 functions
void dht11_init();
bool dht11_read(uint8_t *humidity, uint8_t *temperature);
bool dht11_check_response();
uint8_t dht11_read_byte();

// Set the GPIO pin as input or output as needed
void gpio_set_direction_output(uint8_t pin) {
    TUYA_GPIO_NUM_E pin_id = pin;
    const TUYA_GPIO_BASE_CFG_T io_conf = {
        .direct = TUYA_GPIO_OUTPUT,
        .mode = TUYA_GPIO_PUSH_PULL,
        .level = TUYA_GPIO_LEVEL_LOW,
    };
    tkl_gpio_init(pin_id, &io_conf);
}

void gpio_set_direction_input(uint8_t pin) {
    TUYA_GPIO_NUM_E pin_id = pin;
    const TUYA_GPIO_BASE_CFG_T io_conf = {
        .direct = TUYA_GPIO_INPUT,
        .mode = TUYA_GPIO_PULLUP,
        .level = TUYA_GPIO_LEVEL_LOW,
    };
    tkl_gpio_init(pin_id, &io_conf);
}

void gpio_set_low(uint8_t pin) {
    tkl_gpio_write(pin, TUYA_GPIO_LEVEL_LOW);
}

void gpio_set_high(uint8_t pin) {
    tkl_gpio_write(pin, TUYA_GPIO_LEVEL_HIGH);
}

bool gpio_read(uint8_t pin) {
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(pin, &level);
    return level;
}

void delay_us(unsigned int us) {
    // tal_system_sleep(us/1000);
    US_DELAY(us);
}

// Implement DHT11 initialization
void dht11_init() {
    gpio_set_direction_output(GPIO_PIN);  // Set GPIO as output to initiate the start signal
    // gpio_set_high(GPIO_PIN);              // Pull the line high initially
}

// Function to read from DHT11 sensor
bool dht11_read(uint8_t *humidity, uint8_t *temperature) {
    uint8_t data[5] = {0};  // Array to hold the 40 bits received from DHT11

    // Start signal: pull the line low for 18 ms, then high for 20-40 us
    gpio_set_direction_output(GPIO_PIN);
    gpio_set_low(GPIO_PIN);   // Pull line low
    delay_us(18000);          // Wait for at least 18ms
    gpio_set_high(GPIO_PIN);  // Pull line high
    delay_us(40);             // Wait for 20-40µs

    // Set GPIO to input mode and wait for sensor's response
    gpio_set_direction_input(GPIO_PIN);
    if (!dht11_check_response()) {
        TAL_PR_ERR("DHT11 response not detected!");
        return false;  // Sensor did not respond correctly
    }

    // Read the 5 bytes (40 bits) from the DHT11 sensor
    for (int i = 0; i < 5; i++) {
        data[i] = dht11_read_byte();
    }

    // Check the checksum (last byte should be the sum of the first four)
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        TAL_PR_ERR("DHT11 checksum error!");
        return false;
    }

    // Parse the data for humidity and temperature (DHT11 format)
    *humidity = data[0];      // Humidity is in the first byte (no decimal places)
    *temperature = data[2];   // Temperature is in the third byte (no decimal places)

    return true;
}

// Check response from DHT11 sensor
bool dht11_check_response() {
    delay_us(80);  // Wait for 80µs
    if (!gpio_read(GPIO_PIN)) {  // Check if the line is pulled low by the sensor
        delay_us(80);  // Wait for the next 80µs
        if (gpio_read(GPIO_PIN)) {  // Check if the line is high (sensor presence pulse)
            return true;  // Sensor responded
        }
    }
    return false;  // Sensor did not respond
}

// Read a byte from DHT11 (8 bits)
uint8_t dht11_read_byte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        // Wait for the start of the bit transmission
        while (!gpio_read(GPIO_PIN));  // Wait for line to go high (start of bit)

        // Delay to determine whether it's a 0 or 1 (timing-dependent)
        delay_us(30);  // DHT11 pulls high for ~26-28µs for 0, ~70µs for 1

        if (gpio_read(GPIO_PIN)) {
            byte |= (1 << (7 - i));  // Set the corresponding bit if it's a '1'
        }

        // Wait for the line to go low again (end of bit transmission)
        while (gpio_read(GPIO_PIN));
    }
    return byte;
}

int dht11_test() {
    dht11_init();  // Initialize the DHT11 sensor
    
    uint8_t humidity;
    uint8_t temperature;
    
    if (dht11_read(&humidity, &temperature)) {
        TAL_PR_DEBUG("Temperature: %d°C, Humidity: %d%%\n", temperature, humidity);
    } else {
        TAL_PR_ERR("Failed to read from DHT11 sensor.");
    }

    return 0;
}

void delay_test()
{
    gpio_set_high(GPIO_PIN);
    US_DELAY(5000);
    gpio_set_low(GPIO_PIN);
    US_DELAY(5000);

}
