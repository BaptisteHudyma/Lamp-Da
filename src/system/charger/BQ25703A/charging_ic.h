#ifndef CHARGING_IC_H
#define CHARGING_IC_H

#include <cstdint>
#include <optional>
namespace BQ25703A {

enum Status_t {
  // not initialized
  UNINITIALIZED,
  // works fine
  NOMINAL,

  // generic error
  ERROR,
  // wrong manufacturer/device id
  ERROR_COMPONENT,
  // the device is signaling faults
  ERROR_HAS_FAULTS,
};

enum ChargeStatus_t {
  // no charging
  OFF,
  // battery undervoltage charge
  PRECHARGE,
  // nominal charge
  FASTCHARGE,

  // activated when the charging current is too low
  SLOW_CHARGE
};

// call on program start
bool enable(const uint16_t minSystemVoltage_mV,
            const uint16_t maxBatteryVoltage_mV,
            const uint16_t minInputVoltage_mV,
            const uint16_t maxChargingCurrent_mA,
            const bool forceReset = false);

// call every loop turn
// isChargeOk should match what is read on the CHRG_OK IC pin
void loop(const bool isChargeOk);

// call on program stop
void shutdown();

// enable the charger
// this will start the charge only if the conditions are right
void enable_charge(const bool enable);

// set the input current limit
void set_input_current_limit(const uint16_t maxInputCurrent_mA,
                             const bool shouldUseICO);

// return true if an input source is present for the charger
bool is_input_source_present();

// try to clear the faults that can be cleared.
// if this succeeds, the status will be to NOMINAL next loop call
void try_clear_faults();

// return the status of the component
Status_t get_status();
// return the charge status of the battery
ChargeStatus_t get_charge_status();

// store the DAC values
struct Measurments {
  // the time those measurments were made
  uint32_t time_ms = 0;
  // do not use the values if they are invalid
  bool is_measurment_valid() const;

  // voltage on vbus
  uint16_t vbus_mV;
  // power on vsys ??
  uint16_t psys_mV;
  // battery charging current
  uint16_t batChargeCurrent_mA;
  // battery discharge current
  uint16_t batDischargeCurrent_mA;
  // vbus current
  uint16_t vbus_mA;
  //
  uint16_t cmpin_mA;
  // VSYS voltage
  uint16_t vsys_mV;
  // battery voltage
  uint16_t battery_mV;
};
Measurments get_measurments();

struct Battery {
  // if false, the other infos are useless
  bool isPresent = false;

  // voltage of the battery
  uint16_t voltage_mV;
  // actual current use
  // positive: charging
  // negative: discharging
  int16_t current_mA;
};
Battery get_battery();

}  // namespace BQ25703A

#endif