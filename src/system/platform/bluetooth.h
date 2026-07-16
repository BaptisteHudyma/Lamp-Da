/*! \file bluetooth.h
    \brief Interface for the platform specific bluetooth.
*/

#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>
#include <string>

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
/// Return true if the connected device is paired
bool is_secured();

/// start the advertising sequence (with a timeout)
/// \param[in] isOpenToAll If true, this device can be seen and connected to by every other BLE device. False will use a
/// whitelist of devices, if available, or fail.
void start_advertising(const bool isOpenToAll);

// disable the bluetooth advertising, but not the bluetooth
void stop_bluetooth_advertising();

// update battery level
void write_battery_level(const uint8_t batteryLevel);
void notify_battery_level(const uint8_t batteryLevel);

} // namespace bluetooth
} // namespace platform
} // namespace lampda

#endif
