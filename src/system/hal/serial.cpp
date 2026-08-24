#ifndef HAL_SERIAL_CPP
#define HAL_SERIAL_CPP

#include "serial.h"

#include <Arduino.h>

namespace lampda {
namespace hal {
namespace serial {

void init() { Serial.begin(115200); }
bool is_activated() { return true; }
bool is_available() { return Serial.available() ? true : false; }
char read() { return (char)Serial.read(); }
size_t write(const char* const buffer, size_t bufferSize) { return Serial.write(buffer, bufferSize); }

uint16_t mtu_size() { return 256; }

} // namespace serial
} // namespace hal
} // namespace lampda

#endif
