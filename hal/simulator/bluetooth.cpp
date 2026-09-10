/*! \file bluetooth.cpp
    \brief bluetooth for the simulator
*/

#include "src/system/hal/bluetooth.h"

namespace simulator::bluetooth {
bool isStarted = false;
bool hasBoundedPeer = false;
bool isAdvertising = false;

bool isAdvertisingOpenToAll = false;

bool isConnected = false;

} // namespace simulator::bluetooth

namespace lampda {
namespace hal {
namespace bluetooth {

bool is_activated() { return simulator::bluetooth::isStarted; }

bool is_advertising() { return simulator::bluetooth::isAdvertising; }

bool is_open_to_all() { return simulator::bluetooth::isAdvertisingOpenToAll; }

bool is_connected() { return false; }

bool is_bounded() { return simulator::bluetooth::hasBoundedPeer; }

bool is_bounded(std::array<uint8_t, 8>& buffer) { return simulator::bluetooth::hasBoundedPeer; }

void disconnect() { simulator::bluetooth::isConnected = false; }

bool is_connection_allowed(uint16_t connectionHandle) { return true; }

void load_bound_file() { simulator::bluetooth::hasBoundedPeer = true; }

void clear_bounded_devices() { simulator::bluetooth::hasBoundedPeer = false; }

// start the advertising sequence (with a timeout)
void start_advertising(bool allowUnknownConnections)
{
  simulator::bluetooth::isAdvertising = true;
  simulator::bluetooth::isAdvertisingOpenToAll = allowUnknownConnections;
}

// disable the bluetooth controler
void stop_bluetooth_advertising() { simulator::bluetooth::isAdvertising = false; }

void write_battery_level(const uint8_t batteryLevel) {}

void notify_battery_level(const uint8_t batteryLevel) {}

bool was_used() { return false; }

void shutdown() {}

namespace serial {
bool is_activated() { return hal::bluetooth::is_activated(); }

bool is_available() { return false; }

char read() { return '\n'; }

size_t write(const char* const buffer, size_t bufferSize) { return bufferSize; }

uint16_t mtu_size() { return 20; }

} // namespace serial

} // namespace bluetooth
} // namespace hal
} // namespace lampda
