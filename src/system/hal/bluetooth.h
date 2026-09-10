/*! \file bluetooth.h
    \brief Interface for the platform specific bluetooth.
*/

#ifndef HAL_BLUETOOTH_HPP
#define HAL_BLUETOOTH_HPP

#include <stdint.h>
#include <string>
#include <array>

#include "src/system/bsp/text_in.h"

namespace lampda {
namespace hal {
/// Handle the platform specific bluetooth operations
namespace bluetooth {

// Load bounded devices
void load_bound_file();

/// Return true if the bluetooth is activated
bool is_activated();
/// Return true is the bluetooth is visible by other devices
bool is_advertising();
/// Return true if the device is advertising, and all can connect
bool is_open_to_all();
/// Return true if a bluetooth user is connected
bool is_connected();
/// Return true if the bluetooth connection is bounded to a pair
bool is_bounded();
bool is_bounded(std::array<uint8_t, 8>& buffer);

/// Force disconnection
void disconnect();

/// If this returns false, do not accept any actions from bluetooth
bool is_connection_allowed(uint16_t connectionHandle);

/// Clear the bounded bluetooth devices
void clear_bounded_devices();

/**
 * \brief start the advertising sequence (with a timeout)
 * \param[in] allowUnknownConnections If true, allow any device to be connected. If false, it will first check if a
 * saved device exists, and if it's the case will only allow this device to connect.
 */
void start_advertising(bool allowUnknownConnections);

/// disable the bluetooth advertising, but not the bluetooth
void stop_bluetooth_advertising();

/// update battery level
void write_battery_level(const uint8_t batteryLevel);
/// notify the connected device of a battery level
void notify_battery_level(const uint8_t batteryLevel);

/// Return tue if the bluetooth was used during lifetime
bool was_used();

/// Optional bypass to start the BLE device early, without advertising
void init();

/// shutdown the bluetooth and services
void shutdown();

namespace serial {
/// Return true if the serial port is active
bool is_activated();

/// Return true if a char is available to read
bool is_available();

/// Read a character (blocking)
char read();

/// Write a buffer to bluetooth serial
size_t write(const char* const buffer, size_t bufferSize);

/// Return the usable MTU size
uint16_t mtu_size();

} // namespace serial

} // namespace bluetooth
} // namespace hal
} // namespace lampda

#endif
