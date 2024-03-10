#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <stdint.h>

namespace bluetooth {

// call once when the program starts
void startup_sequence();

// enable the bluetooth controler
void enable_bluetooth();

// disable the bluetooth controler
void disable_bluetooth();

// call at each loop turn
void parse_messages();

};  // namespace bluetooth

#endif