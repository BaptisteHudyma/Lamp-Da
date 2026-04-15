/*! \file charger.h
    \brief High level layer of the battery charging and balancer algorithms.
    The charger can be in charge or OTG mode.
    Charge transfers power from the power rail to the battery, and OTG transfers power from the battery to the power
   rail.
*/

#ifndef CHARGER_H
#define CHARGER_H

#include <cstdint>
#include <string>

/// Handles the battery charging processes, and power output control.
namespace charger {

/// call once at program startup
bool setup();
/// call at every loop runs
void loop();
/// Call when system goes to sleep
void shutdown();

/// debug feature: enable/disable the charging process
void set_enable_charge(const bool shouldCharge);

/**
 * \brief Store the charger characteristics
 */
struct Charger_t
{
  /// everything below makes no sense if this is false
  bool areMeasuresOk = false;
  /// True if we have a valid chargeOk signal from the component
  bool isChargeOkSignalHigh;
  /// True if OTG is activated.
  bool isInOtg;

  /// time of the last update of this struct.
  uint32_t lastUpdateTime_ms = 0;

  /// input current in VBUS side, in milliamps.
  uint16_t inputCurrent_mA = 0;
  /// charge current of the battery, in milliamps.
  uint16_t chargeCurrent_mA = 0;
  /// powerRail voltage (can be the same as VBUS in charge & OTG mode).
  /// Min value at 3200mV
  uint16_t powerRail_mV = 0;

  /// battery voltage
  uint16_t batteryVoltage_mV = 0;
  /// battery current (> 0 charging, < 0 discharging)
  int16_t batteryCurrent_mA = 0;

  /**
   * \brief Indicates a charger status soecific states
   */
  enum class ChargerStatus_t
  {
    /// not initialized yet
    UNINITIALIZED,

    /// no input power, charger is down
    INACTIVE,
    /// charger detects power on the input
    POWER_DETECTED,
    /// charging but very slowly
    SLOW_CHARGING,
    /// currently charging the batteries
    CHARGING,
    /// charging process is done
    CHARGE_FINISHED,

    /// battery not detected
    ERROR_BATTERY_MISSING,
    /// a software error was detected
    ERROR_SOFTWARE,
    /// critical: some hardware is faulty
    ERROR_HARDWARE
  };
  /// global high level charger status
  ChargerStatus_t status = ChargerStatus_t::UNINITIALIZED;

  /// Return true when the status is charging or powered
  bool is_charging() const;
  /// Return true when the battery is truly charging
  bool is_effectivly_charging() const;
  /// Return true if the charge is finished
  bool is_charge_finished() const;
  /// Return the status as a string
  std::string get_status_str() const;

  /// Contain a more detailed error of an hardware error
  std::string hardwareErrorMessage = "";
  /// Contain a more detailed error of a software error
  std::string softwareErrorMessage = "";
};

/// Return true if voltage is detected on VBUS.
bool is_vbus_powered();

/// Return true if we are allowed to use the VBUS power.
bool can_use_vbus_power();

/**
 * \brief Set the OTG control parameters. Set the parameters to zero to disable OTG mode.
 * \param[in] mv Desired output voltage in millivolts
 * \param[in] ma Desired maximum output current in milliamps
 */
void control_OTG(const uint16_t mv, const uint16_t ma);

/// Return true if the USB VBUS presence signal is active.
bool is_vbus_signal_detected();

/// Return the latest known state of the charger.
Charger_t get_state();

} // namespace charger

#endif // CHARGER_H
