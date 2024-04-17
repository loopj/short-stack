#include "led.h"

#include <util/atomic.h>
#include <util/delay.h>

// Calculate the number of CPU cycles for a given time in microseconds
#define CYCLES(time_us) (int)((time_us * F_CPU / 1000000) + 0.5)

// Delay a given number of CPU cycles
#define DELAY_CYCLES(count) if (count > 0) __builtin_avr_delay_cycles(count);

// Static buffer to store color data for each LED
static uint8_t led_buffer[LED_COUNT * LED_BYTES] = {0};

void led_init()
{
    // Set the LED pin as an output and set it low
    LED_PORT.DIRSET = (1 << LED_PIN);
    LED_PORT.OUTCLR = (1 << LED_PIN);
}

void led_set_color(uint8_t index, uint32_t color)
{
    for (uint8_t i = 0; i < LED_BYTES; i++) {
        led_buffer[index * LED_BYTES + i] = (color >> (8 * ((LED_BYTES - 1) - i))) & 0xFF;
    }
}

void led_refresh()
{
    // Disable interrupts while sending data
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        // Send each byte of data
        for (uint16_t c = 0; c < LED_COUNT * LED_BYTES; c++) {
            // Send each bit of the byte, MSB first
            const uint8_t val = led_buffer[c];
            for (int8_t b = 7; b >= 0; b--) {
                if ((val >> b) & 0x1) {
                    // Send a 1
                    LED_PORT.OUTSET = (1 << LED_PIN);
                    DELAY_CYCLES(CYCLES(LED_T1H) - 1);
                    LED_PORT.OUTCLR = (1 << LED_PIN);
                    DELAY_CYCLES(CYCLES(LED_T1L) - 1);
                } else {
                    // Send a 0
                    LED_PORT.OUTSET = (1 << LED_PIN);
                    DELAY_CYCLES(CYCLES(LED_T0H) - 1);
                    LED_PORT.OUTCLR = (1 << LED_PIN);
                    DELAY_CYCLES(CYCLES(LED_T0L) - 1);
                }
            }
        }
    }

    // Wait for the latch time before sending more data, so LEDs update
    // NOTE: We aren't doing high frequency updates, so we don't need to wait
    DELAY_CYCLES(CYCLES(LED_LAT));
}