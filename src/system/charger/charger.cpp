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

constexpr uint16_t baseChargeCurrent_mA = 128;

PD_UFP_c PD_UFP;

String status = "";

static bool isChargeEnabled_s = true;
void enable_charger() {
  // set charger to low impedance mode (enable charger)
  if (!isChargeEnabled_s) {
    isChargeEnabled_s = true;

    BQ25703Areg.chargeOption1.set_FORCE_LATCHOFF(0);
    charger.writeRegEx(BQ25703Areg.chargeOption1);

    // BQ25703Areg.chargeOption3.set_EN_HIZ(0);
    //  BQ25703Areg.chargeOption3.set_BATFETOFF_HIZ(0);
    BQ25703Areg.chargeOption3.set_EN_OTG(0);
    charger.writeRegEx(BQ25703Areg.chargeOption3);
  }
}

void disable_charger() {
  // set charger to high impedance mode (disable charger)
  if (isChargeEnabled_s) {
    isChargeEnabled_s = false;
    BQ25703Areg.chargeOption1.set_FORCE_LATCHOFF(1);
    charger.writeRegEx(BQ25703Areg.chargeOption1);

    // BQ25703Areg.chargeOption3.set_EN_HIZ(1);
    //  BQ25703Areg.chargeOption3.set_BATFETOFF_HIZ(1);
    BQ25703Areg.chargeOption3.set_EN_OTG(0);
    charger.writeRegEx(BQ25703Areg.chargeOption3);

    // always deactivate low power mode for wake up
    BQ25703Areg.chargeOption0.set_EN_LWPWR(1);
    charger.writeRegEx(BQ25703Areg.chargeOption0);
  }
}

// enable or disable the ADC to measure values
void setChargerADC(const bool activate) {
  const uint8_t val = activate ? 1 : 0;

  // Set the ADC on IBAT to record values
  BQ25703Areg.chargeOption1.set_EN_IBAT(val);
  charger.writeRegEx(BQ25703Areg.chargeOption1);

  // Set ADC to make continuous readings. (uses more power)
  BQ25703Areg.aDCOption.set_ADC_CONV(val);
  // Set ADC for each parameters
  BQ25703Areg.aDCOption.set_EN_ADC_CMPIN(val);
  BQ25703Areg.aDCOption.set_EN_ADC_VBUS(val);
  BQ25703Areg.aDCOption.set_EN_ADC_PSYS(val);
  BQ25703Areg.aDCOption.set_EN_ADC_IIN(val);
  BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(val);
  BQ25703Areg.aDCOption.set_EN_ADC_ICHG(val);
  BQ25703Areg.aDCOption.set_EN_ADC_VSYS(val);
  BQ25703Areg.aDCOption.set_EN_ADC_VBAT(val);
  // write the whole register
  charger.writeRegEx(BQ25703Areg.aDCOption);
}

void setup() {
  // reset all registers of charger
  // BQ25703Areg.chargeOption3.set_RESET_REG(1);
  // charger.writeRegEx(BQ25703Areg.chargeOption3);
  disable_charger();

  PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_20V);

  // first step, reset charger parameters
  disable_charge();

  status.reserve(100);
}

void shutdown() { disable_charge(); }

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

bool is_powered_on() {
  // is charger powered on (charge signal ok)
  return (digitalRead(CHARGE_OK) == HIGH);
}

// check if the charger can use the max power (vbus voltage == negociated
// power)
bool can_use_max_power() {
  // if the negociated power is available, use it !
  return PD_UFP.get_vbus_voltage() > (PD_UFP.get_voltage() * 50 - 2000);
}

static bool isCharging_s = true;
bool is_charging() { return isCharging_s; }

bool charge_processus() {
  static bool isChargeEnabled = false;
  static bool isChargeResetted = false;

  // prevent flickering when the charge is almost over
  static bool voltageHysteresisActivated = false;

  static uint8_t lastChargeValue = 0;
  static uint32_t lastBatteryRead = 0;
  const uint8_t batteryLevel = battery::get_battery_level();

  if (batteryLevel > lastChargeValue) {
    lastChargeValue = batteryLevel;
    lastBatteryRead = millis();
  }

  bool shouldCharge = true;

  // voltage hystereis is active, do not charge
  if (voltageHysteresisActivated) {
    shouldCharge = false;
    // do not start charge back until we drop below the target threshold
    if (batteryLevel < 90) {
      voltageHysteresisActivated = false;
      lastBatteryRead = 0;
      lastChargeValue = 0;
      status = "NOT CHARGING: voltage hysteresys is being disabled";
    } else {
      status = "NOT CHARGING: voltage hysteresys activated";
    }
  } else {
    // battery level stuck, stop charge (even if not full)
    constexpr uint32_t safeTimeoutMin = 25;

    // battery level high and stable: stop charge
    constexpr uint32_t timeoutMin = 15;

    const uint32_t timestamp = millis();
    if (batteryLevel >= 95) {
      if (timestamp - lastBatteryRead > 60 * 1000 * timeoutMin) {
        status = "NOT CHARGING: charge finished";

        shouldCharge = false;
        voltageHysteresisActivated = true;
      }
    } else if (timestamp - lastBatteryRead > 60 * 1000 * safeTimeoutMin) {
      status = "NOT CHARGING: charge timeout: battery level stuck";

      shouldCharge = false;
    }
  }

  // temperature too high, stop charge
  if ((AlertManager.current() & Alerts::TEMP_CRITICAL) != 0x00) {
    status = "NOT CHARGING: temperature critical";

    shouldCharge = false;
  }

  // no power on VBUS, no charge
  if (!is_powered_on() && !PD_UFP.is_vbus_ok()) {
    status = "NOT CHARGING: vbus level not ok";

    shouldCharge = false;
  }
  // else: normal voltages

  static uint32_t startChargeEnabledTime = 0;
  // should not charge battery, or charge is not enabled
  if (!shouldCharge) {
    startChargeEnabledTime = 0;
    isCharging_s = false;
    isChargeEnabled = false;

    // stop charge already invoked ?
    if (not isChargeResetted) {
      disable_charge();
      isChargeResetted = true;
    }

    // dont run the charge functions
    return false;
  }
  // reset the reset flag
  isChargeResetted = false;
  // reset hysteresis
  voltageHysteresisActivated = false;

  // charge part of the program
  if (not isChargeEnabled) {
    status = "CHARGING: starting negociation";

    //  reset pd negociation
    PD_UFP.reset();

    // Set the watchdog timer to have a short timeout
    BQ25703Areg.chargeOption0.set_WDTMR_ADJ(1);  // timeout 5 seconds
    charger.writeRegEx(BQ25703Areg.chargeOption0);

    // enable all ADCs
    setChargerADC(true);

    startChargeEnabledTime = millis();
    isChargeEnabled = true;

    BQ25703Areg.maxChargeVoltage.set_voltage(16750);
  }

  //  run pd negociation
  PD_UFP.run();

  isCharging_s = true;

  uint16_t chargeCurrent_mA = baseChargeCurrent_mA;  // default usb voltage
  if (PD_UFP.is_USB_PD_available()) {
    if (can_use_max_power()) {
      status = "CHARGING: USB PD power negociated";
      // get_current returns values in cA instead of mA, so must do x10
      chargeCurrent_mA = PD_UFP.get_current() * 10;
    }
    // else: wait for power to climb
  } else {
    // do not start charge without PD before one second
    if (millis() - startChargeEnabledTime > 2000) {
      status = "CHARGING: standard charging mode after no charger answer";
      chargeCurrent_mA = 500;  // fallback current, for all chargeurs
    }
  }

  // set charger to low impedance mode (enable charger)
  if (chargeCurrent_mA > baseChargeCurrent_mA) {
    enable_charger();

    // update the charge current (max charge current is defined by the battery
    // used)
    BQ25703Areg.chargeCurrent.set_current(
        min(batteryMaxChargeCurrent, chargeCurrent_mA));
  }

  // flag to signal that the charge must be stopped
  isChargeResetted = false;

  return true;
}

void disable_charge() {
  // reset the pd negociation
  PD_UFP.reset();

  disable_charger();

  setChargerADC(false);
  BQ25703Areg.chargeCurrent.set_current(baseChargeCurrent_mA);
}

String charge_status() { return status; }

uint16_t getVbusVoltage_mV() { return PD_UFP.get_vbus_voltage(); }

}  // namespace charger