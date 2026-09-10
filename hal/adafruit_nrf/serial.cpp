#include "src/system/hal/serial.h"

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"

namespace lampda {
namespace hal {
namespace serial {

void init() { Serial.begin(115200); }

bool is_activated()
{
  const bool isSerial = Serial;
  return isSerial;
}

bool is_available() { return Serial.available() ? true : false; }

char read() { return (char)Serial.read(); }

size_t write(const char* const buffer, size_t bufferSize) { return Serial.write(buffer, bufferSize); }

uint16_t mtu_size() { return Serial.availableForWrite(); }

} // namespace serial
} // namespace hal
} // namespace lampda
