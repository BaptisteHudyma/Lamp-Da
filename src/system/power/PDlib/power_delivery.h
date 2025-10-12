#ifndef PD_POWER_DELIVERY_H
#define PD_POWER_DELIVERY_H

#include <cstdint>
#include <vector>
namespace powerDelivery {

// call once at program start, start all threads
bool setup();
void start_threads();

// call often (update status)
void loop();

// call once at program end
void shutdown();

void suspend_pd_state_machine();
void resume_pd_state_machine();

// use the vbus measure from negociator (close to USBC, 0 to N volts)
int get_vbus_voltage();

// return the max current available for this source
uint16_t get_max_input_current();

// return true if this voltage source is from a standard non pd port
bool is_standard_port();

// return true is a cbale is connected
// may not be connected to anything
bool is_cable_detected();

// some power available on VBUS
bool is_power_available();

// can use this source as power entry
bool can_use_power();

/// force the system to source mode
void force_set_to_source_mode(const bool force);

/**
 * \brief Call to allow or forbid OTG mode
 */
void allow_otg(const bool);

/**
 * \brief return true is the system is prepaping to switch to OTG mode
 */
bool is_switching_to_otg();

struct PDOTypes
{
  uint32_t voltage_mv;
  uint32_t maxCurrent_mA;
};

/**
 * \brief If the charger is PD compatible, return it's capabilities
 */
std::vector<PDOTypes> get_available_pd();
void show_pd_status();

// return the requested OTG parameters
struct OTGParameters
{
  uint16_t requestedVoltage_mV = 0;
  uint16_t requestedCurrent_mA = 0;

  bool is_otg_requested() const { return requestedVoltage_mV != 0 && requestedCurrent_mA != 0; }

  static OTGParameters get_default()
  {
    OTGParameters def;
    def.requestedCurrent_mA = 3000;
    def.requestedVoltage_mV = 5000;
    return def;
  }
};
OTGParameters get_otg_parameters();

} // namespace powerDelivery

#endif
