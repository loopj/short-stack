#pragma once

#include <stdint.h>

// Configure the RTC for ~1ms periodic interrupts
void rtc_init();

// Get the current millisecond count since the RTC was started
uint32_t rtc_millis();