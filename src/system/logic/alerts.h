/*! \file alerts.h
    \brief Handle the main alerts behavior.
*/

#ifndef ALERTS_H
#define ALERTS_H

#include <atomic>
#include <cstdint>

#include "src/system/utils/constants.h"

namespace lampda {
namespace logic {
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

  OTG_OVER_LIMITS = 1 << 19, ///< OTG has gone over the limits (current or voltage)
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

  /**
   * \brief Clear an alert, unless it was refreshed by a raise() since the clear decision was taken.
   * \param[in] type The alert to clear
   * \param[in] refreshCount The refresh counter of this alert, incremented by every raise() of a raised alert
   * \param[in] refreshCountAtDecision Value of \p refreshCount read before deciding to clear the alert
   * \return true if the alert was cleared
   */
  bool clear_unless_refreshed(const Type type,
                              const std::atomic<uint32_t>& refreshCount,
                              const uint32_t refreshCountAtDecision);

  /// Return true if the target alert is raised
  bool is_raised(const Type type) const { return (_current.load() & static_cast<uint32_t>(type)) != 0x00; }

  /// Return true if no alerts are raised
  bool is_clear() const { return _current.load() == 0x00; }

  /// the time in milliseconds since this alert was raised, or zero if it's not. This raise time is updated by a raise()
  /// function call.
  uint32_t get_time_since_raised(const Type type) const;

  /// Return true if a raised alert prevents power set to output
  bool can_use_output_power() const;

  /// Return true if a raised alert prevents battery charging
  bool can_charge_battery() const;

  /// Return true if no raised alerts block the use of power through USB port
  bool can_use_usb_port() const;

  AlertManager_t() = default;
  AlertManager_t(const AlertManager_t&) = delete;
  AlertManager_t& operator=(AlertManager_t const&) = delete;

private:
  /// Binary variable to store raised alerts.
  /// Alerts are raised and cleared from several threads: only use atomic read-modify-write operations on it
  std::atomic<uint32_t> _current {0};
};

/// external global reference to the AlertManager
extern AlertManager_t manager;

/// Signal slot that the system just powered up from charger
void signal_wake_up_from_charger();

/**
 * \brief Signal a fault of the USB port protection. Safe to call from an interrupt: it only sets a flag.
 * The USB_PORT_SHORT alert is raised (or refreshed) by the next process_interrupt_signals() call.
 */
void signal_usb_port_fault();

/// Raise the alerts signaled from interrupts since the last call. Called by the alerts loop and the power thread
void process_interrupt_signals();

/// handle the behavior for all alerts. Must be called often
void handle_all(const bool shouldIgnoreAlerts);

/// Return the max output brightness allowed by the raised alerts (the lowest of their limits)
brightness_t get_alerts_max_brightness();

/// show all the raised alerts in Serial debug
void show_all();

/// Return true if an alert requested an immediate shutdown. Handling this is urgent or dammages can happen.
bool is_request_shutdown();

} // namespace alerts
} // namespace logic
} // namespace lampda

#endif
