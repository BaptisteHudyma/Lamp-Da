#ifndef ALERTS_H
#define ALERTS_H

#include <cstdint>

// 32 errors max
enum Alerts
{
  NONE = 0, // system is sane and ready

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
};

class Alert
{
public:
  void raise_alert(const Alerts alert)
  {
    if (alert == Alerts::NONE)
      return;
    _current |= alert;
  }

  void clear_alert(const Alerts alert)
  {
    if ((_current & alert) == 0x0)
      return;
    _current ^= alert;
  }

  uint32_t current() const { return _current; }

private:
  uint32_t _current;
};

extern Alert AlertManager;

#endif