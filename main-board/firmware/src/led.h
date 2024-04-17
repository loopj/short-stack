/**
 * Quick and dirty addressable LED library for AVR 0/1-series MCUs
 *
 * - Bit-bangs data to addressable LEDs
 * - Supports 1 wire addressable LEDs with WS2812B-ish protocols
 * - Supports a single chain of LEDs
 */

#pragma once

#include <stdint.h>

// LED layout configuration
#define LED_COUNT 3 // Number of LEDs in the chain
#define LED_BYTES 3 // Number of bytes per LED (eg. 3 for RGB)

// LED timing configuration (values in microseconds)
// Everlight 19-C47 / 12-23C LEDs
// https://jp.everlight.com/wp-content/uploads/2021/02/One-Wire-RGBIC-Application-note-V1.0EN.pdf
#define LED_T0H 0.3
#define LED_T0L 0.9
#define LED_T1H 0.9
#define LED_T1L 0.3
#define LED_LAT 50

// LED data pin configuration
#define LED_PORT PORTC
#define LED_PIN  2

// Initialize the LED data pin
void led_init();

// Set the color of an LED in the buffer
void led_set_color(uint8_t index, uint32_t color);

// Refresh the LEDs with the current buffer
void led_refresh();