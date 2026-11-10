/*! \file power_delivery.h
    \brief Interface for the abstraction layer of the USB-PD algorithm.
*/

#ifndef BSP_PD_POWER_DELIVERY_H
#define BSP_PD_POWER_DELIVERY_H

#include <cstdint>
#include <array>

namespace lampda {
namespace bsp {
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

/**
 * \brief Allow the sink to reset the source, when the source capabilities were missed.
 * A hard reset makes the source drop VBUS for up to ~2s. Forbid it when the battery cannot power the board (recovery
 * mode), or this browns it out: the source is then used as a standard 5V port.
 * Forbidden until allowed. Allowing it later starts the reset of a source waiting for its capabilities.
 */
void allow_source_resets(const bool allow);

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
 * \brief As a sink, ask the partner for the source role (power role swap), and do not use its power meanwhile
 * \param[in] wanted false to go back to the normal sink behavior
 */
void want_source_role_by_swap(const bool wanted);

/// return true when the PD state machine is attached to a partner, as a sink
bool is_attached_as_sink();

/// return true when we are a source, with a contract in place (PS_RDY sent): the sink can use the power
bool is_source_contract_ready();

/**
 * \brief return true is the system is prepaping to switch to OTG mode
 */
bool is_switching_to_otg();

/**
 * \brief return true when we are a source in a hard reset: VBUS must be cut until the end of the recovery time
 */
bool is_source_hard_resetting();

/**
 * \brief Define a USB PD PDO type.
 */
struct PDOTypes
{
  /// Indicate if this object contains valid data
  bool is_valid() const
  {
    return voltage_mv >= 3500 && voltage_mv <= 22000 && maxCurrent_mA > 0 && maxCurrent_mA <= 5000;
  }

  /// Voltage of this PDO
  uint32_t voltage_mv = 0;
  /// Maximum current of this PDO
  uint32_t maxCurrent_mA = 0;
};

/**
 * \brief If the charger is PD compatible, return it's capabilities
 */
std::array<PDOTypes, 5> get_available_pd();

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
} // namespace bsp
} // namespace lampda

#endif
