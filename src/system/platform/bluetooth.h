/*! \file bluetooth.h
    \brief Interface for the platform specific bluetooth.
*/

#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>
#include <string>

#include "src/compile.h"
// - contains #define USE_BLUETOOTH

namespace lampda {
namespace platform {
/// Handle the platform specific bluetooth operations
namespace bluetooth {

/// Return true if the bluetooth is activated
bool is_activated();
/// Return true is the bluetooth is visible by other devices
bool is_advertising();
/// Return true if a bluetooth user is connected
bool is_connected();

// start the advertising sequence (with a timeout)
void start_advertising();

// disable the bluetooth advertising, but not the bluetooth
void stop_bluetooth_advertising();

// update battery level
void write_battery_level(const uint8_t batteryLevel);
void notify_battery_level(const uint8_t batteryLevel);

} // namespace bluetooth
} // namespace platform
} // namespace lampda

#endif
