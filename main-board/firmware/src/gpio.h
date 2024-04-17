#pragma once

#include <avr/io.h>
#include <stdbool.h>

// Struct to represent an AVR pin
typedef struct gpio {
    PORT_t *port;
    uint8_t num;
} gpio_t;

// Set the direction of a pin to output
static inline void gpio_output(gpio_t gpio)
{
    gpio.port->DIRSET = (1 << gpio.num);
}

// Set the direction of a pin to input
static inline void gpio_input(gpio_t gpio)
{
    gpio.port->DIRCLR = (1 << gpio.num);
}

// Set gpio configuration (see PORT_PINnCTRL in the datasheet)
static inline void gpio_config(gpio_t gpio, uint8_t ctrl)
{
    (&gpio.port->PIN0CTRL)[gpio.num] = ctrl;
}

// Set a gpio high
static inline void gpio_set_high(gpio_t gpio)
{
    gpio.port->OUTSET = (1 << gpio.num);
}

// Set a gpio low
static inline void gpio_set_low(gpio_t gpio)
{
    gpio.port->OUTCLR = (1 << gpio.num);
}

// Toggle a gpio
static inline void gpio_toggle(gpio_t gpio)
{
    gpio.port->OUTTGL = (1 << gpio.num);
}

// Read the state of a gpio
static inline bool gpio_read(gpio_t gpio)
{
    return (gpio.port->IN & (1 << gpio.num));
}

// Read the state of an interrupt flag
static inline bool gpio_read_intflag(gpio_t gpio)
{
    return (gpio.port->INTFLAGS & (1 << gpio.num));
}