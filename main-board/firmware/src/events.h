#pragma once

#include <stdint.h>

static volatile uint8_t events = 0;

static inline void event_set(uint8_t event)
{
    events |= (1 << event);
}

static inline bool event_get(uint8_t event)
{
    if (events & (1 << event)) {
        events &= ~(1 << event);
        return true;
    }
    return false;
}