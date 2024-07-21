#include "charger.h"

#include <cstdint>

#include "../alerts.h"
#include "../physical/BQ25703A.h"
#include "../physical/battery.h"
#include "../utils/constants.h"
#include "Arduino.h"
#include "FUSB302/PD_UFP.h"

namespace charger {

// Initialise the device and library
bq2573a::BQ25703A charger;

// Create instance of registers data structure
bq2573a::BQ25703A::Regt BQ25703Areg;

PD_UFP_c PD_UFP;

void setup() {
  BQ25703Areg.chargeCurrent.set_current(150);  // charge current regulation

  // set the pd negociation
  PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_20V);

  // run first pd negociation
  PD_UFP.run();
}

bool check_vendor_device_values() {
  byte manufacturerId = BQ25703Areg.manufacturerID.get_manufacturerID();
  if (manufacturerId != bq2573a::MANUFACTURER_ID) {
    // wrong manufacturer id
    return false;
  }

  byte deviceId = BQ25703Areg.deviceID.get_deviceID();
  if (deviceId != bq2573a::DEVICE_ID) {
    // wrong device id
    return false;
  }

  return true;
}

bool is_usb_powered() {
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00;
}

bool is_powered_on() { return (digitalRead(CHARGE_OK) == HIGH); }

// check if the charger can use the max power (vbus voltage == negociated power)
bool can_use_max_power() {
  return BQ25703Areg.aDCVBUSPSYS.get_VBUS() >
         (PD_UFP.get_voltage() * 50 - 2000);
}

bool enable_charge() {
  static bool isChargeEnabled = false;
  static bool isChargeResetted = false;

  // prevent flickering when the charge is almost over
  static bool voltageHysteresisActivated = false;

  bool shouldCharge = true;
  // battery high enough;: stop charge
  if (battery::get_battery_level() >= 99) {
    shouldCharge = false;
    voltageHysteresisActivated = true;
  }
  // do not start charge back until we drop below the target threshold
  else if (voltageHysteresisActivated) {
    shouldCharge = false;
    if (battery::get_battery_level() < 97) {
      voltageHysteresisActivated = false;
    }
  }

  // temperature too high, stop charge
  if ((AlertManager.current() & Alerts::TEMP_CRITICAL) != 0x00) {
    shouldCharge = false;
  }

  // should not charge battery, or charge is not enabled
  if (!shouldCharge) {
    // stop charge already invoked ?
    if (not isChargeResetted) {
      disable_charge();
      isChargeResetted = true;
    }

    // dont run the charge functions
    return false;
  }

  // reset hysteresis
  voltageHysteresisActivated = false;

  // Setting the max voltage that the charger will charge the batteries up
  // to
  if (is_powered_on()) {
    if (not isChargeEnabled) {
      isChargeEnabled = true;

      // set the pd negociation (also serve as a reset)
      PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_20V);

      // Setting the max voltage that the charger will charge the batteries up
      // to
      BQ25703Areg.maxChargeVoltage.set_voltage(16750);  // max battery voltage
      BQ25703Areg.chargeCurrent.set_current(150);  // charge current regulation

      // Set the ADC on IBAT to record values
      BQ25703Areg.chargeOption1.set_EN_IBAT(1);
      charger.writeRegEx(BQ25703Areg.chargeOption1);

      // Set ADC to make continuous readings. (uses more power)
      BQ25703Areg.aDCOption.set_ADC_CONV(1);
      // Set individual ADC registers to read. All have default off
      BQ25703Areg.aDCOption.set_EN_ADC_CMPIN(1);
      BQ25703Areg.aDCOption.set_EN_ADC_VBUS(1);
      BQ25703Areg.aDCOption.set_EN_ADC_PSYS(1);
      BQ25703Areg.aDCOption.set_EN_ADC_IIN(1);
      BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(1);
      BQ25703Areg.aDCOption.set_EN_ADC_ICHG(1);
      BQ25703Areg.aDCOption.set_EN_ADC_VSYS(1);
      BQ25703Areg.aDCOption.set_EN_ADC_VBAT(1);
      //  Once bits have been twiddled, send bytes to device
      charger.writeRegEx(BQ25703Areg.aDCOption);

      BQ25703Areg.maxChargeVoltage.set_voltage(16750);
    }

    // run pd negociation
    PD_UFP.run();

    uint16_t chargeCurrent_mA = 100;  // default usb voltage
    if (can_use_max_power()) {
      // get_current returns values in cA instead of mA, so must do x10
      chargeCurrent_mA = PD_UFP.get_current() * 10;
    }

    // update the charge current (max charge current is defined by the battery
    // used)
    BQ25703Areg.chargeCurrent.set_current(
        min(batteryMaxChargeCurrent, chargeCurrent_mA));

    // flag to signal that the charge must be stopped
    isChargeResetted = false;
  } else {
    isChargeEnabled = false;
  }

  return isChargeEnabled;
}

void disable_charge() {
  // set charge current to 0
  BQ25703Areg.chargeOption1.set_EN_IBAT(0);
  charger.writeRegEx(BQ25703Areg.chargeOption1);
  BQ25703Areg.aDCOption.set_ADC_CONV(0);
  BQ25703Areg.aDCOption.set_EN_ADC_CMPIN(0);
  BQ25703Areg.aDCOption.set_EN_ADC_VBUS(0);
  BQ25703Areg.aDCOption.set_EN_ADC_PSYS(0);
  BQ25703Areg.aDCOption.set_EN_ADC_IIN(0);
  BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(0);
  BQ25703Areg.aDCOption.set_EN_ADC_ICHG(0);
  BQ25703Areg.aDCOption.set_EN_ADC_VSYS(0);
  BQ25703Areg.aDCOption.set_EN_ADC_VBAT(0);
  charger.writeRegEx(BQ25703Areg.aDCOption);

  BQ25703Areg.chargeCurrent.set_current(0);
}

void set_system_voltage(const float voltage) {
  const uint16_t v = voltage * 1000.0;
  BQ25703Areg.minSystemVoltage.set_voltage(v);
}

}  // namespace charger