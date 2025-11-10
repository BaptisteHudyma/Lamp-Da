#ifndef CHARGER_H
#define CHARGER_H

#include <cstdint>
#include <string>

namespace charger {

// call once at program startup
bool setup();
// call at every loop runs
void loop();

void shutdown();

// debug feature: disable the charging process
void set_enable_charge(const bool shouldCharge);

struct Charger_t
{
  // everything below makes no sense if this is false
  bool areMeasuresOk = false;

  bool isChargeOkSignalHigh;
  bool isInOtg;

  // input current in VBUS side
  uint16_t inputCurrent_mA = 0;
  // charge current of the battery
  uint16_t chargeCurrent_mA = 0;
  // powerRail voltage (can be the same as VBUS in charge & OTG mode)
  // min value at 3200mV
  uint16_t powerRail_mV = 0;

  // battery voltage
  uint16_t batteryVoltage_mV = 0;
  // battery current (> 0 charging, < 0 discharging)
  int16_t batteryCurrent_mA = 0;

  enum class ChargerStatus_t
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
  bool is_effectivly_charging() const;
  bool is_charge_finished() const;
  std::string get_status_str() const;

  // should contain a more detailed error
  std::string hardwareErrorMessage = "";
  std::string softwareErrorMessage = "";
};

bool is_vbus_powered();
bool can_use_vbus_power();

// set the otg parameters
// set to 0 to deactivate
void control_OTG(const uint16_t mv, const uint16_t ma);

// the microcontroler is detecting a vbus input voltage
bool is_vbus_signal_detected();

Charger_t get_state();

} // namespace charger

#endif // CHARGER_H
