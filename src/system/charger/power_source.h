#ifndef POWER_SOURCE_H
#define POWER_SOURCE_H

#include <cstdint>
namespace powerSource {

// call once at program start
bool setup();

//
void loop();

// call once at program end
void shutdown();

// return the max current available for this source
uint16_t get_max_input_current();

// return true if this voltage source is from a standard non pd port
bool is_standard_port();

// some power available on VBUS
bool is_power_available();

// can use this source as power entry
bool can_use_power();

// return the requested OTG parameters
struct OTGParameters
{
  uint16_t requestedVoltage_mV;
  uint16_t requestedCurrent_mA;
};
OTGParameters get_otg_parameters();

} // namespace powerSource

#endif