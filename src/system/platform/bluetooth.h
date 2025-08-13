#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>

#include "src/compile.h"
// - contains #define USE_BLUETOOTH

namespace bluetooth {

// start the advertising sequence (with a timeout)
void start_advertising();

// disable the bluetooth advertising, but not the bluetooth
void stop_bluetooth_advertising();

// update battery level
void write_battery_level(const uint8_t batteryLevel);
void notify_battery_level(const uint8_t batteryLevel);

}; // namespace bluetooth

#endif
