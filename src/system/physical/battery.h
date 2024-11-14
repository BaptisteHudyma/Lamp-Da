#ifndef BATTERY_H
#define BATTERY_H

#include "Arduino.h"

namespace battery {

// return a number between 0 and 100
// it corresponds to the real battery level
extern uint8_t get_raw_battery_level(const bool resetRead = false);

// returns the battery level, corresponding to user safe choice (0-100)
extern uint8_t get_battery_level(const bool resetRead = false);

extern void raise_battery_alert();

}  // namespace battery

#endif