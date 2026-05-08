/*! \file bluetooth_mock.cpp
    \brief Mock of the board bluetooth
*/

#include "src/system/platform/bluetooth.h"

namespace lampda {
namespace platform {
namespace bluetooth {

bool is_activated() { return false; }

bool is_advertising() { return false; }

bool is_connected() { return false; }

// start the advertising sequence (with a timeout)
void start_advertising() {}

// disable the bluetooth controler
void stop_bluetooth_advertising() {}

void write_battery_level(const uint8_t batteryLevel) {}

void notify_battery_level(const uint8_t batteryLevel) {}

} // namespace bluetooth
} // namespace platform
} // namespace lampda
