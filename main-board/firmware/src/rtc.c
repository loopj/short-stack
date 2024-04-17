#include "rtc.h"

#include <avr/interrupt.h>
#include <avr/io.h>

static volatile uint32_t millis = 0;

// Handle periodic interrupts on the RTC
ISR(RTC_PIT_vect)
{
    RTC.PITINTFLAGS = RTC_PI_bm;
    millis++;
}

void rtc_init()
{
    RTC.CLKSEL     = RTC_CLKSEL_INT32K_gc;
    RTC.PITINTCTRL = RTC_PI_bm;
    RTC.PITCTRLA   = RTC_PERIOD_CYC32_gc | RTC_PITEN_bm;
}

uint32_t rtc_millis()
{
    return millis;
}