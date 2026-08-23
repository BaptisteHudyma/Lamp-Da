/*! \file bluetooth_mock.cpp
    \brief Mock of the board bluetooth
*/

#include "src/system/hal/bluetooth.h"

namespace lampda {
namespace hal {
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

bool send_uart(char const* buffer) { return true; }

bool was_used() { return false; }

void shutdown() {}

namespace serial {
bool is_activated() { return hal::bluetooth::is_activated(); }

bool is_available() { return false; }

char read() { return '\n'; }
} // namespace serial

} // namespace bluetooth
} // namespace hal
} // namespace lampda
