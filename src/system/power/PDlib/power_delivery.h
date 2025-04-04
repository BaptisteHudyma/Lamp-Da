#ifndef PD_POWER_DELIVERY_H
#define PD_POWER_DELIVERY_H

#include <cstdint>
namespace powerDelivery {

// call once at program start
bool setup();

//
void loop();

// call once at program end
void shutdown();

// use the vbus measure from negociator (close to USBC, 0 to N volts)
int get_vbus_voltage();

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
  uint16_t requestedVoltage_mV = 0;
  uint16_t requestedCurrent_mA = 0;

  bool is_otg_requested() const { return requestedVoltage_mV != 0 && requestedCurrent_mA != 0; }
};
OTGParameters get_otg_parameters();

} // namespace powerDelivery

#endif