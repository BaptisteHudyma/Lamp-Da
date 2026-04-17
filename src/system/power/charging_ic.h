/*! \file charging_ic.h
    \brief Interface for the abstraction layer of the battery charger component.
*/

#ifndef CHARGING_IC_H
#define CHARGING_IC_H

#include <cstdint>
#include <string>

/// Handles the battery charging processes.
namespace charger {

/// Hardware driver interactions for the charger component.
namespace drivers {

/// Store the status of the charge component.
enum class Status_t
{
  /// not initialized
  UNINITIALIZED,
  /// works fine
  NOMINAL,
  /// generic error
  ERROR,
  /// wrong manufacturer/device id
  ERROR_COMPONENT,
  /// the device is signaling faults
  ERROR_HAS_FAULTS,
};

/// Status of the charge process
enum class ChargeStatus_t
{
  /// no charging
  OFF,
  /// battery undervoltage charge
  PRECHARGE,
  /// nominal charge
  FASTCHARGE,
  /// activated when the charging current is too low
  SLOW_CHARGE
};

/**
 * \brief Call once on program start
 * \param[in] minSystemVoltage_mV Minimal allowed battery voltage.
 * \param[in] maxBatteryVoltage_mV Maximal allowed battery voltage
 * \param[in] maxChargingCurrent_mA Maximum allowed charge current for the battery.
 * \param[in] maxDichargingCurrent_mA Maximum allowed discharging current of the battery.
 * \param[in] forceReset If true, will reset the internal registers of the component.
 * \return True if the initialization suceeded.
 */
bool enable(const uint16_t minSystemVoltage_mV,
            const uint16_t maxBatteryVoltage_mV,
            const uint16_t maxChargingCurrent_mA,
            const uint16_t maxDichargingCurrent_mA,
            const bool forceReset = false);

/**
 * \brief Call often during execution.
 * \param[in] isChargeOk Should match what is read on the CHRG_OK IC pin of the charger.
 */
void loop(const bool isChargeOk);

/// call on program stop to gracefully shutdown the component.
void shutdown();

/// Enable the charge process.
/// this will start the charge only if the conditions are right
void enable_charge(const bool enable);

/**
 * \brief Set the input current limit
 * \param[in] maxInputCurrent_mA Maximum allowed current use on VBUS, in milliamps.
 * \param[in] shouldUseICO If True, the InputCurrentInitialization will be used. It automatically finds the most allowed
 * current.
 */
void set_input_current_limit(const uint16_t maxInputCurrent_mA, const bool shouldUseICO);

/// return the actual charge current, in milliamps.
uint16_t get_charge_current();

/// return true if an input source is present for the charger on the USB port.
bool is_input_source_present();

/// Enable the OTG and disable the charging process.
void enable_OTG();
/// Disable the OTG.
void disable_OTG();
/// set the desired OTG capabilities
void set_OTG_targets(const uint16_t voltage_mV, const uint16_t maxCurrent_mA);
/// Return true if the system is in OTG mode.
bool is_in_OTG();

/// try to clear the faults that can be cleared.
/// if this succeeds, the status will be to NOMINAL next loop call.
void try_clear_faults();

/// return the status of the component
Status_t get_status();
/// return a string with details on the error status
std::string get_status_detail();
/// return the charge status object of the battery
ChargeStatus_t get_charge_status();
/// contains details on software error if any
std::string get_software_error_message();

/**
 * \brief Store the DAC values mesured by some system sensors, relative to the battery and power.
 */
struct Measurments
{
  /// The time those measurments were made
  uint32_t time = 0;
  /// Return False if you should not use the values in this struct
  bool is_measurment_valid() const;

  /// Indicates if the chargeOk signal from charger is high
  bool isChargeOk;

  /// voltage on vbus, in millivolts
  uint16_t vbus_mV;
  /// power on vsys ??
  /// The values here are always weird
  uint16_t psys_mV;
  /// battery charging current, in milliamps
  uint16_t batChargeCurrent_mA;
  /// battery discharge current, in milliamps
  uint16_t batDischargeCurrent_mA;
  /// vbus current, in milliamps
  uint16_t vbus_mA;
  ///
  uint16_t cmpin_mA;
  /// VSYS voltage, in millivolts
  uint16_t vsys_mV;
  /// battery voltage, in millivolts
  uint16_t battery_mV;
};
Measurments get_measurments();

/**
 * \brief Store the battery specific measurments ans status.
 * It is a refined data version of the Measurments class
 */
struct Battery
{
  /// if false, the other infos are useless
  bool isPresent = false;

  /// voltage of the battery, in millivolts.
  uint16_t voltage_mV;
  /// Actual current use, in milliamps.
  /// Positive is charging current.
  /// Negative is discharging current.
  int16_t current_mA;
};
Battery get_battery();

} // namespace drivers
} // namespace charger

#endif
