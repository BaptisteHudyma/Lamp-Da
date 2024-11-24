#ifndef POWER_SOURCE_H
#define POWER_SOURCE_H

#include <cstdint>
namespace powerSource {

// call once at program start
bool setup();

//
void loop();

// return the max current available for this source
uint16_t get_max_input_current();

// return true if this voltage source is not from power delivery
bool is_not_usb_power_delivery();

// some power available on VBUS
bool is_power_available();

// is the microcontroler powered by vbus
bool is_powered_with_vbus();

} // namespace powerSource

#endif