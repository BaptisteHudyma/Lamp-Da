/*! \file power_delivery.h
    \brief Interface for the abstraction layer of the USB-PD algorithm.
*/

#ifndef PD_POWER_DELIVERY_H
#define PD_POWER_DELIVERY_H

#include <cstdint>
#include <vector>

/// Handles the power delivery capabilities (USB-PD).
namespace powerDelivery {

/// call once at program start, attach interrupts and init
bool setup();
/// Start polling threads
void start_threads();

/// call often (update status)
void loop();

/// call once at program end
void shutdown();

/// Suspend the execution of the power delivery state machine
void suspend_pd_state_machine();
/// resume the execution of the power delivery state machine
void resume_pd_state_machine();

/// use the vbus measure from negociator (close to USBC, 0 to N volts)
int get_vbus_voltage();

/// return the max current available for this source
uint16_t get_max_input_current();

/// return true if this voltage source is from a standard non pd port
bool is_standard_port();

/// Return true is a power cable is connected.
bool is_cable_detected();

/// Return true if some power is available on VBUS
bool is_power_available();

/// Return true if we can use this source as power entry
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

/**
 * \brief Define a USB PD PDO type.
 */
struct PDOTypes
{
  /// Voltage of this PDO
  uint32_t voltage_mv;
  /// Maximum current of this PDO
  uint32_t maxCurrent_mA;
};

/**
 * \brief If the charger is PD compatible, return it's capabilities
 */
std::vector<PDOTypes> get_available_pd();

/// Debug power delivery status
void show_pd_status();

/// The requested OTG parameters
struct OTGParameters
{
  /// Requested voltage for the OTG
  uint16_t requestedVoltage_mV = 0;
  /// Requested maximum current use for the OTG
  uint16_t requestedCurrent_mA = 0;

  /// Return true if OTG is requested and should be turned on
  bool is_otg_requested() const { return requestedVoltage_mV != 0 && requestedCurrent_mA != 0; }

  /**
   * \brief Return the default OTG targets
   */
  static OTGParameters get_default()
  {
    OTGParameters def;
    def.requestedCurrent_mA = 3000;
    def.requestedVoltage_mV = 5000;
    return def;
  }
};

/// Return the desired OTG parameters
OTGParameters get_otg_parameters();

} // namespace powerDelivery

#endif
