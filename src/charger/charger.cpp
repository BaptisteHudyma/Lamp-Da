#include "charger.h"

#include "../physical/BQ25703A.h"
#include "../physical/battery.h"

namespace charger {

// Initialise the device and library
BQ25703A charger;

// Create instance of data structure
BQ25703A::Regt BQ25703Areg;

bool check_vendor_device_values() {
  byte manufacturerId = BQ25703Areg.manufacturerID.get_manufacturerID();
  if (manufacturerId != MANUFACTURER_ID) {
    // wrong manufacturer id
    return false;
  }

  byte deviceId = BQ25703Areg.deviceID.get_deviceID();
  if (deviceId != DEVICE_ID) {
    // wrong device id
    return false;
  }

  return true;
}

bool is_powered_on() { return (digitalRead(CHARGE_OK) == HIGH); }

bool is_charge_enabled() {
  static bool isChargeEnabled = false;
  // TODO: check
  if (!isChargeEnabled) {
    isChargeEnabled = true;
  }

  if (!is_powered_on() or get_battery_level() >= 98) isChargeEnabled = false;
  return isChargeEnabled;
}

void enable_charge() {
  static bool isValueReseted = false;
  if (get_battery_level() >= 98) {
    // set charge current to 0
    if (not isValueReseted) {
      BQ25703Areg.chargeOption1.set_EN_IBAT(0);
      charger.writeRegEx(BQ25703Areg.chargeOption1);
      BQ25703Areg.aDCOption.set_ADC_CONV(0);
      BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(0);
      BQ25703Areg.aDCOption.set_EN_ADC_ICHG(0);
      charger.writeRegEx(BQ25703Areg.aDCOption);

      BQ25703Areg.chargeCurrent.set_current(64);
      isValueReseted = true;
    }
    return;
  } else {
    isValueReseted = false;
  }

  // Setting the max voltage that the charger will charge the batteries up
  // to
  if (!is_charge_enabled() and is_powered_on()) {
    // Setting the max voltage that the charger will charge the batteries up
    // to
    BQ25703Areg.maxChargeVoltage.set_voltage(16400);  // max battery voltage
    BQ25703Areg.chargeCurrent.set_current(1000);  // charge current regulation

    // Set the watchdog timer to not have a timeout
    // When changing bitfield values, call the writeRegEx function
    // This is so you can change all the bits you want before sending out
    // the byte.
    BQ25703Areg.chargeOption0.set_WDTMR_ADJ(0);
    charger.writeRegEx(BQ25703Areg.chargeOption0);

    // Set the ADC on IBAT to record values
    BQ25703Areg.chargeOption1.set_EN_IBAT(1);
    charger.writeRegEx(BQ25703Areg.chargeOption1);

    // Set ADC to make continuous readings. (uses more power)
    BQ25703Areg.aDCOption.set_ADC_CONV(1);
    // Set individual ADC registers to read. All have default off.
    BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(1);
    BQ25703Areg.aDCOption.set_EN_ADC_ICHG(1);
    // Once bits have been twiddled, send bytes to device
    charger.writeRegEx(BQ25703Areg.aDCOption);

    BQ25703Areg.maxChargeVoltage.set_voltage(16400);
  }

  BQ25703Areg.chargeCurrent.set_current(1000);
}

}  // namespace charger