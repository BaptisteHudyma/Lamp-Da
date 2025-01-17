#ifndef ALERTS_H
#define ALERTS_H

#include <cstdint>
#include "src/system/utils/print.h"

// 31 errors max
enum Alerts
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

  OTG_ACTIVATED = 1 << 9, // OTG activated
  OTG_FAILED = 1 << 10,   // OTG activation failed
};

inline const char* AlertsToText(const Alerts alert)
{
  switch (alert)
  {
    case MAIN_LOOP_FREEZE:
      return "MAIN_LOOP_FREEZE";
    case BATTERY_READINGS_INCOHERENT:
      return "BATTERY_READING_INCOHERENT";
    case BATTERY_CRITICAL:
      return "BATTERY_CRITICAL";
    case BATTERY_LOW:
      return "BATTERY_LOW";
    case LONG_LOOP_UPDATE:
      return "LONG_LOOP_UPDATE";
    case TEMP_TOO_HIGH:
      return "TEMP_TOO_HIGH";
    case TEMP_CRITICAL:
      return "TEMP_CRITICAL";
    case BLUETOOTH_ADVERT:
      return "BLUETOOTH_ADVERT";
    case HARDWARE_ALERT:
      return "HARDWARE_ALERT";
    case OTG_ACTIVATED:
      return "OTG_ACTIVATED";
    case OTG_FAILED:
      return "OTG_FAILED";
    default:
      return "UNSUPPORTED TYPE";
  }
}

class Alert
{
public:
  void raise_alert(const Alerts alert)
  {
    if (is_raised(alert))
      return;

    lampda_print("ALERT raised: %s", AlertsToText(alert));
    _current |= alert;
  }

  void clear_alert(const Alerts alert)
  {
    if (not is_raised(alert))
      return;
    lampda_print("ALERT cleared: %s", AlertsToText(alert));
    _current ^= alert;
  }

  /**
   * \brief Return true if an alert is raised
   */
  bool is_raised(const Alerts alert) const { return (_current & alert) != 0x00; }

  /**
   * \brief Return true if no alerts are raised
   */
  bool is_clear() const { return _current == 0x00; }

private:
  uint32_t _current;
};

extern Alert AlertManager;

#endif