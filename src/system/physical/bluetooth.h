#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>

#include "src/compile.h"
// - contains #define USE_BLUETOOTH

namespace bluetooth {

// start the advertising sequence (with a timeout)
void start_advertising();

// disable the bluetooth controler
void disable_bluetooth();

// call at each loop turn
void parse_messages();

}; // namespace bluetooth

#endif
