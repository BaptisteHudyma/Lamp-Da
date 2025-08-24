#ifndef ALERTS_H
#define ALERTS_H

#include <cstdint>

namespace alerts {

// 31 errors max
enum Type : uint32_t
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
};

class AlertManager_t
{
public:
  void raise(const Type type);

  void clear(const Type type);

  /**
   * \brief Return true if an alert is raised
   */
  bool is_raised(const Type type) const { return (_current & type) != 0x00; }
  bool is_any_alert_raised() const { return _current != 0x00; }

  /**
   * \brief Return true if no alerts are raised
   */
  bool is_clear() const { return _current == 0x00; }

private:
  uint32_t _current;
};

extern AlertManager_t manager;

// call to signal that the system just powered up from charger
void signal_wake_up_from_charger();

// handle the behavior for all alerts
void handle_all(const bool shouldIgnoreAlerts);
// show all the raised alerts
void show_all();
// return true if an alert requested an immediate shutdown
bool is_request_shutdown();

} // namespace alerts

#endif
