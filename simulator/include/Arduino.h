#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

//
// minimal Arduino.h replacement
//

#include <string>
#include <type_traits>

#include <cstdint>
#include <cmath>

// constants
using String = std::string;

//
// lampda-specific hacks
//

using byte = uint8_t;

//
// ambiguous abs
//

using std::abs;

//
// emulate time_ms()
//

static uint64_t globalMillis;

#endif
