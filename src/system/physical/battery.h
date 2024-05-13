#ifndef BATTERY_H
#define BATTERY_H

#include "Arduino.h"

namespace battery {

// return a number between 0 and 100
extern uint8_t get_battery_level(const bool resetRead = false);

extern void raise_battery_alert();

}  // namespace battery

#endif