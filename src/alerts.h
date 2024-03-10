#ifndef ALERTS_H
#define ALERTS_H

#include <cstdint>
enum Alerts {
  NONE = 0,  // system is sane and ready

  // always sort them by importance
  MAIN_LOOP_FREEZE = 0b0000001,  // main loop does not respond
  BATTERY_READINGS_INCOHERENT =
      0b0000010,  // the pin that reads the battery value is not coherent with
                  // it's givent min and max
  BATTERY_CRITICAL = 0b0000100,  // battery is too low, shutdown immediatly
  BATTERY_LOW = 0b0001000,       // battery is dangerously low
  LONG_LOOP_UPDATE = 0b0010000,  // the main loop is taking too long to execute
                                 // (bugs when reading button inputs)
  UNKNOWN_COLOR_MODE = 0b0100000,   // An incorrect color mode was reached
  UNKNOWN_COLOR_STATE = 0b1000000,  // An incorrect color state was reached
};

class Alert {
 public:
  void raise_alert(const Alerts alert) {
    if (alert == Alerts::NONE) return;
    _current |= alert;
  }

  void clear_alert(const Alerts alert) {
    if ((_current & alert) == 0x0) return;
    _current ^= alert;
  }

  uint32_t current() const { return _current; }

 private:
  uint32_t _current;
};

extern Alert AlertManager;

#endif