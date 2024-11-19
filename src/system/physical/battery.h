#ifndef BATTERY_H
#define BATTERY_H

#include "Arduino.h"

namespace battery {

// return a number between 0 and 10000 (% * 100)
// it corresponds to the real battery level
extern uint16_t get_raw_battery_level();

// returns the battery level, corresponding to user safe choice (0-10000)
extern uint16_t get_battery_level();

extern void raise_battery_alert();

} // namespace battery

#endif