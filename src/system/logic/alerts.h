/*! \file alerts.h
    \brief Handle the main alerts behavior.
*/

#ifndef ALERTS_H
#define ALERTS_H

#include <cstdint>

/// Handle the system alerts, that are displayed on the RGB indicator
namespace alerts {

/// Alert types :  31 errors max
enum class Type : uint32_t
{
  // 0 means no errors

  // always sort them by importance
  MAIN_LOOP_FREEZE = 1 << 0,            // main loop does not respond
  BATTERY_READINGS_INCOHERENT = 1 << 1, // the pin that reads the battery value is not coherent with
                                        // it's givent min and max
  BATTERY_CRITICAL = 1 << 2,            // battery is too low, shutdown immediatly
  BATTERY_LOW = 1 << 3,                 // battery is dangerously low
  LONG_LOOP_UPDATE = 1 << 4,            // the main loop is taking too long to execute
                                        // (bugs when reading button inputs)
  TEMP_TOO_HIGH = 1 << 5,               // Processor temperature is too high
  TEMP_CRITICAL = 1 << 6,               // Processor temperature is critical

  BLUETOOTH_ADVERT = 1 << 7, // bluetooth is advertising

  HARDWARE_ALERT = 1 << 8, // any hardware alert

  FAVORITE_SET = 1 << 9, // user favorite mode is set

  OTG_FAILED = 1 << 10, // OTG activation failed

  SYSTEM_OFF_FAILED = 1 << 11,     // system failed to go to sleep, big trouble here
  SYSTEM_IN_ERROR_STATE = 1 << 12, // system is locked in an error state

  SYSTEM_IN_LOCKOUT = 1 << 13, // system lockout, the lamp should not output any light

  SUNSET_TIMER_ENABLED = 1 << 14, // active sunset timer, system will auto turn off

  SYSTEM_SLEEP_SKIPPED = 1 << 15, // the system skipped the sleep clean phase (crash ? new flash ?)

  USB_PORT_SHORT = 1 << 16, // the usb port is dirty, or wet

  BATTERY_MISSING = 1 << 17, // no battery plugged in the system

  CHARGER_ERROR = 1 << 18, // the charger signaled an error
};

/**
 * \brief Handle the whole alerts logic. Use this to raise and clear alerts, and make decisions based on those alerts.
 * Alerts can be raised and cleared on their own.
 */
class AlertManager_t
{
public:
  /// Raise an alert
  void raise(const Type type);

  /// Clear an alert
  void clear(const Type type);

  /// Return true if the target alert is raised
  bool is_raised(const Type type) const { return (_current & static_cast<uint32_t>(type)) != 0x00; }

  /// Return true if no alerts are raised
  bool is_clear() const { return _current == 0x00; }

  /// the time in milliseconds since this alert was raised, or zero if it's not. This raise time is updated by a raise()
  /// function call.
  uint32_t get_time_since_raised(const Type type) const;

  /// Return true if a raised alert prevents power set to output
  bool can_use_output_power() const;

  /// Return true if a raised alert prevents battery charging
  bool can_charge_battery() const;

  /// Return true if no raised alerts block the use of power through USB port
  bool can_use_usb_port() const;

private:
  /// Binary variable to store raised alerts
  uint32_t _current;
};

/// external global reference to the AlertManager
extern AlertManager_t manager;

/// Signal slot that the system just powered up from charger
void signal_wake_up_from_charger();

/// handle the behavior for all alerts. Must be called often
void handle_all(const bool shouldIgnoreAlerts);

/// show all the raised alerts in Serial debug
void show_all();

/// Return true if an alert requested an immediate shutdown. Handling this is urgent or dammages can happen.
bool is_request_shutdown();

} // namespace alerts

namespace indicator {

/**
 * \brief Set the brightness of the RGB indicator.
 * Indicator level can be
 * 0 : indicator and alerts displayed as 100% brightness
 * 1 : indicator off, alerts displayed as 25% brightness
 * 2 : indicator and alerts displayed as 25% brightness
 */
void set_brightness_level(const uint8_t level);
/**
 * \brief Get the brightness of the RGB indicator.
 * Indicator level can be
 * 0 : indicator and alerts displayed as 100% brightness
 * 1 : indicator off, alerts displayed as 25% brightness
 * 2 : indicator and alerts displayed as 25% brightness
 */
uint8_t get_brightness_level();

} // namespace indicator

#endif
