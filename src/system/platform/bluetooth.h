#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>

#include "src/compile.h"
// - contains #define USE_BLUETOOTH

namespace bluetooth {

// start the advertising sequence (with a timeout)
void start_advertising();

// disable the bluetooth controler
void disable_bluetooth();

// update battery level
void write_battery_level(const uint8_t batteryLevel);
void notify_battery_level(const uint8_t batteryLevel);

}; // namespace bluetooth

#endif
