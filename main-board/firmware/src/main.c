#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

#include "events.h"
#include "gpio.h"
#include "led.h"
#include "rtc.h"

// GPIO definitions
static const gpio_t FAN_PWM    = {&PORTA, 3};
static const gpio_t REG_EN     = {&PORTA, 5};
static const gpio_t THERM      = {&PORTA, 6};
static const gpio_t POWER      = {&PORTA, 7};
static const gpio_t BT_PWR_REQ = {&PORTB, 4};
static const gpio_t SHUTDOWN   = {&PORTB, 5};
static const gpio_t PWR_BTN    = {&PORTC, 1};

// LED definitions
static const uint8_t POWER_LED  = 0;
static const uint8_t DISC_LED_L = 1;
static const uint8_t DISC_LED_R = 2;

// Output pulse timings
static const uint16_t POWER_PULSE_MS = 30;

// Button press thresholds
static const uint16_t BTN_PRESS_MS = 10;
static const uint16_t BTN_HOLD_MS  = 2000;

// Button states
enum button_state { BUTTON_RELEASED, BUTTON_PRESSED, BUTTON_HELD };

// Flags for events detected in interrupt handlers
enum event_flags { PWR_BTN_PRESS_EVENT, PWR_BTN_HOLD_EVENT, BT_PWR_REQ_EVENT, SHUTDOWN_EVENT };

// State
bool regs_enabled          = false;
bool power_pulse_active    = false;
uint32_t power_pulse_start = 0;

// Enable or disable the main regulators, updating the LEDs accordingly
void set_regulator_state(bool enabled)
{
    if (enabled) {
        // Turn on main regulators
        gpio_set_high(REG_EN);

        // Turn on the fan
        gpio_set_high(FAN_PWM);

        // Set power indicator LED to green, disc LEDs to cyan
        led_set_color(POWER_LED, 0x002000);
        led_set_color(DISC_LED_L, 0x00FFFF);
        led_set_color(DISC_LED_R, 0x00FFFF);
    } else {
        // Turn off main regulators
        gpio_set_low(REG_EN);

        // Turn off the fan
        gpio_set_low(FAN_PWM);

        // Set power indicator LED to red, turn off disc LEDs
        led_set_color(POWER_LED, 0x200000);
        led_set_color(DISC_LED_L, 0x000000);
        led_set_color(DISC_LED_R, 0x000000);
    }

    // Refresh the LEDs
    led_refresh();

    // Update state
    regs_enabled = enabled;
}

// Update the state of the power button
void update_pwr_btn_events()
{
    static bool last_gpio_state           = true;
    static uint32_t last_millis           = 0;
    static enum button_state button_state = BUTTON_RELEASED;

    // Read the current state of the power button (active low)
    bool gpio_state = gpio_read(PWR_BTN);

    // Reset hold time and flags if the button state has changed
    uint32_t millis = rtc_millis();
    if (gpio_state != last_gpio_state) {
        last_millis  = millis;
        button_state = BUTTON_RELEASED;
    }

    // If the button is down, check for hold time thresholds
    if (gpio_state == false) {
        // Raise event flags when hold time thresholds are reached
        if (millis - last_millis >= BTN_PRESS_MS && button_state < BUTTON_PRESSED) {
            button_state = BUTTON_PRESSED;
            event_set(PWR_BTN_PRESS_EVENT);
        } else if (millis - last_millis >= BTN_HOLD_MS && button_state < BUTTON_HELD) {
            button_state = BUTTON_HELD;
            event_set(PWR_BTN_HOLD_EVENT);
        }
    }

    // Save the current state for comparison next time
    last_gpio_state = gpio_state;
}

// Send a pulse to the POWER GPIO to request a soft shutdown
void send_power_pulse()
{
    gpio_set_high(POWER);
    power_pulse_active = true;
    power_pulse_start  = rtc_millis();
}

// Clear the POWER GPIO if the pulse has been active for the required time
void clear_completed_power_pulse()
{
    if (power_pulse_active && rtc_millis() - power_pulse_start >= POWER_PULSE_MS) {
        gpio_set_low(POWER);
        power_pulse_active = false;
    }
}

// Disable the prescaler to run the CPU at full speed
void clk_disable_prescaler()
{
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB &= ~CLKCTRL_PEN_bm;
}

// Initialize the GPIOs
void gpio_init()
{
    // Power button input (active low on button press)
    gpio_input(PWR_BTN);
    gpio_config(PWR_BTN, PORT_PULLUPEN_bm);

    // Bluetooth power request input (active high, ~110ms pulse)
    gpio_input(BT_PWR_REQ);
    gpio_config(BT_PWR_REQ, PORT_ISC_RISING_gc);

    // Soft shutdown complete input (active high, ~120ms pulse)
    gpio_input(SHUTDOWN);
    gpio_config(SHUTDOWN, PORT_ISC_RISING_gc);

    // Regulator enable output
    gpio_output(REG_EN);

    // Soft shutdown request output
    gpio_output(POWER);

    // Fan power output
    gpio_output(FAN_PWM);
}

// Handle external interrupts on PORTB
ISR(PORTB_PORT_vect)
{
    // Check for "bluetooth power request" pulse
    if (gpio_read_intflag(BT_PWR_REQ)) {
        event_set(BT_PWR_REQ_EVENT);
    }

    // Check for "soft shutdown complete" pulse
    if (gpio_read_intflag(SHUTDOWN)) {
        event_set(SHUTDOWN_EVENT);
    }

    // Clear the interrupt flags
    PORTB.INTFLAGS = 0xFF;
}

int main()
{
    // Initialize the RTC with a 1ms periodic interrupt
    // We need this to measure power button press and hold times, and to time output pulses
    rtc_init();

    // Initialize the GPIOs
    gpio_init();

    // Initialize the addressable LEDs
    led_init();

    // Disable the prescaler to run the CPU at full speed
    // Required for accurate bit-banging of LEDs
    clk_disable_prescaler();

    // Enable global interrupts
    sei();

    // Set the initial state of the main regulators to off
    set_regulator_state(false);

    // Main loop
    while (true) {
        // Check for power button events (press, hold)
        update_pwr_btn_events();

        // Check for completed power pulses, and turn off the POWER gpio if necessary
        clear_completed_power_pulse();

        // Handle power button presses
        if (event_get(PWR_BTN_PRESS_EVENT)) {
            if (regs_enabled) {
                // Send a soft shutdown request pulse to Hollywood
                send_power_pulse();
            } else {
                // Turn on the main regulators if they are off
                set_regulator_state(true);
            }
        }

        // Turn off the main regulators if the power button is held for 2 seconds
        if (event_get(PWR_BTN_HOLD_EVENT)) {
            if (regs_enabled)
                set_regulator_state(false);
        }

        // Handle bluetooth power request pulses
        if (event_get(BT_PWR_REQ_EVENT)) {
            if (regs_enabled) {
                // Send a soft shutdown request pulse to Hollywood
                send_power_pulse();
            } else {
                // Turn on the main regulators if they are off
                set_regulator_state(true);
            }
        }

        // Turn off the main regulators if Hollywood indicates a soft shutdown is complete
        if (event_get(SHUTDOWN_EVENT)) {
            if (regs_enabled)
                set_regulator_state(false);
        }
    }

    return 0;
}