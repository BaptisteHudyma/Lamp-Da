#ifndef CHARGER_H
#define CHARGER_H

#include <Arduino.h>

#include <cstdint>

namespace charger {

// call once at program startup
void setup();
// call at every loop runs
void loop();
// call when the system will shutdown
void shutdown();

// debug feature: disable the charging process
void set_enable_charge(const bool shouldCharge);

void enable_OTG();

struct Charger_t
{
  // everything below makes no sense if this is false
  bool areMeasuresOk = false;

  // input current in VBUS side
  uint16_t inputCurrent_mA = 0;
  // charge current of the battery
  uint16_t chargeCurrent_mA = 0;
  // current VBUS voltage
  uint16_t vbus_mV = 0;

  // battery voltage
  uint16_t batteryVoltage_mV = 0;
  // battery current (> 0 charging, < 0 discharging)
  int16_t batteryCurrent_mA = 0;

  enum ChargerStatus_t
  {
    // not initialized yet
    UNINITIALIZED,

    // no input power, charger is down
    INACTIVE,
    // charger detects power on the input
    POWER_DETECTED,
    // charging but very slowly
    SLOW_CHARGING,
    // currently charging the batteries
    CHARGING,
    // charging process is done
    CHARGE_FINISHED,

    // battery not detected
    ERROR_BATTERY_MISSING,
    // a software error was detected
    ERROR_SOFTWARE,
    // critical: some hardware is faulty
    ERROR_HARDWARE
  };
  // global status
  ChargerStatus_t status = ChargerStatus_t::UNINITIALIZED;

  // return true when the status is charging or powered
  bool is_charging() const;
  String get_status_str() const;
};

bool is_vbus_powered();

// the microcontroler is detecting a vbus input voltage
bool is_vbus_signal_detected();

Charger_t get_state();

} // namespace charger

#endif // CHARGER_H