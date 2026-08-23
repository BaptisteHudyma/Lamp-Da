/*! \file bluetooth.h
    \brief Interface for the platform specific bluetooth.
*/

#ifndef HAL_BLUETOOTH_HPP
#define HAL_BLUETOOTH_HPP

#include <stdint.h>
#include <string>

#include "src/system/bsp/text_in.h"

namespace lampda {
namespace hal {
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

// send an uart comman over bluetooth
bool send_uart(char const* buffer);

/// Return tue if the bluetooth was used during lifetime
bool was_used();

// shutdown the bluetooth and services
void shutdown();

namespace serial {
/// Return true if the serial port is active
bool is_activated();

/// Return true if a char is available to read
bool is_available();

/// Read a character (blocking)
char read();
} // namespace serial

} // namespace bluetooth
} // namespace hal
} // namespace lampda

#endif
