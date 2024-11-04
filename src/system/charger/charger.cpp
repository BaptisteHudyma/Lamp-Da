#include "charger.h"

#include <cstdint>

#include "Arduino.h"
#include "FUSB302/PD_UFP.h"
#include "src/system/alerts.h"
#include "src/system/physical/BQ25703A.h"
#include "src/system/physical/battery.h"
#include "src/system/utils/constants.h"

namespace charger {

// Initialise the device and library
bq2573a::BQ25703A charger;

// Create instance of registers data structure
bq2573a::BQ25703A::Regt BQ25703Areg;

// what do we expect from a standard USB vbus
constexpr uint16_t vbusBaseCurrent_mA = 500;
constexpr uint32_t vbusBasePower = 5 * vbusBaseCurrent_mA;

constexpr float chargerEfficiency = 0.85;  // pessimistic estimation

PD_UFP_c PD_UFP;

String status = "";

static bool isChargeEnabled_s = true;
void enable_charger(const bool force) {
  // set charger to low impedance mode (enable charger)
  if (force or !isChargeEnabled_s) {
    isChargeEnabled_s = true;

    BQ25703Areg.chargeOption1.set_FORCE_LATCHOFF(0);
    charger.writeRegEx(BQ25703Areg.chargeOption1);

    // toggle HIGHZ on and off, to prevent a register latch bug
    /*
    BQ25703Areg.chargeOption3.set_EN_HIZ(1);
    BQ25703Areg.chargeOption3.set_BATFETOFF_HIZ(1);
    charger.writeRegEx(BQ25703Areg.chargeOption3);
    */

    BQ25703Areg.chargeOption3.set_EN_HIZ(0);
    BQ25703Areg.chargeOption3.set_BATFETOFF_HIZ(0);
    BQ25703Areg.chargeOption3.set_EN_OTG(0);
    charger.writeRegEx(BQ25703Areg.chargeOption3);
  }
}

void disable_charger(const bool force) {
  // set charger to high impedance mode (disable charger)
  if (force or isChargeEnabled_s) {
    isChargeEnabled_s = false;
    BQ25703Areg.chargeOption1.set_FORCE_LATCHOFF(1);
    charger.writeRegEx(BQ25703Areg.chargeOption1);

    // This breaks the USB detection somehow
    // -> There is a leakage between 3 and 4v on VBUS in HIGHZ mode,
    // wastes energy and breaks USB detection (risk of lock)
    // -> It seems to come from one of the MOSFET on the battery side

    // BQ25703Areg.chargeOption3.set_EN_HIZ(1);
    // BQ25703Areg.chargeOption3.set_BATFETOFF_HIZ(1);

    // end of warning zone

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

void read_alerts() {
  // options
  charger.readRegEx(BQ25703Areg.chargeOption0);
  charger.readRegEx(BQ25703Areg.chargeOption1);
  charger.readRegEx(BQ25703Areg.chargeOption2);
  charger.readRegEx(BQ25703Areg.chargeOption3);

  // processor hot alerts
  charger.readRegEx(BQ25703Areg.prochotOption0);
  charger.readRegEx(BQ25703Areg.prochotOption1);
  charger.readRegEx(BQ25703Areg.prochotStatus);

  charger.readRegEx(BQ25703Areg.aDCOption);
  charger.readRegEx(BQ25703Areg.chargerStatus);
}

void setup() {
  // reset all registers of charger
  BQ25703Areg.chargeOption3.set_RESET_REG(1);
  charger.writeRegEx(BQ25703Areg.chargeOption3);

  PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_5V);

  // first step, reset charger parameters
  disable_charge(true);

  // set target voltage (convert to mv)
  BQ25703Areg.maxChargeVoltage.set(batteryMaxVoltage * 1000);
  // set charge current to 0
  BQ25703Areg.chargeCurrent.set(0);

  status.reserve(100);
  status = "UNINITIALIZED";
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

static bool isCharging_s = false;
bool is_charging() { return isCharging_s; }

static bool isPowerCreepingInProcess_s = false;
/**
 * \brief Slowly raise the charge current until VBUS drops below a threshold
 * This is kind a dangerous, so use responsibly
 * \param[in] batteryVoltage_V current battery voltage in volts
 * \param[in] maxVbusCurrent_mA max current that we can draw on VBUS
 * \return the target charging current to be set
 */
uint16_t regulate_vbus_with_power_creep(const float batteryVoltage_V,
                                        const uint16_t maxVbusCurrent_mA) {
  // no charge !!
  if (!isCharging_s) return 0;

  // power creep should only be used on VBUS at 5V
  const uint16_t vbusVoltage_mV = BQ25703Areg.aDCVBUSPSYS.get_VBUS();
  if (vbusVoltage_mV > 5300) {
    return 0;
  }

  const uint16_t currentChargingCurrent = BQ25703Areg.chargeCurrent.get();

  static uint32_t lastCall = 0;
  const uint32_t time = millis();
  // augment power until voltage starts to drop
  // let some time pass for everything to settle
  if (time - lastCall > 1000 && vbusVoltage_mV > 4500) {
    lastCall = time;
    isPowerCreepingInProcess_s = true;

    const uint32_t possiblyUsablePower =
        ((5 * maxVbusCurrent_mA) * chargerEfficiency) / batteryVoltage_V;
    // slowly augment charging current, clamped by max charging current
    return min(
        possiblyUsablePower,
        currentChargingCurrent + BQ25703Areg.aDCVBUSPSYS.resolutionVal1());
  } else {
    isPowerCreepingInProcess_s = false;
    // stay in the confort zone
    const uint16_t standardUsableCurrent_mA =
        (vbusBasePower * chargerEfficiency) / batteryVoltage_V;

    return max(currentChargingCurrent, standardUsableCurrent_mA);
  }
}

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
    }
    // this is commented out to keep the last charger status
    // else { status = "NOT CHARGING: voltage hysteresys activated"; }
  } else {
    // battery level stuck, stop charge (even if not full)
    constexpr uint32_t safeTimeoutMin = 30;

    // battery level high and stable: stop charge
    constexpr uint32_t timeoutMin = 20;
    const uint32_t timestamp = millis();

    // TODO: also stop charging process if charging current is none.

    // full battery, can stop immediatly
    if (batteryLevel >= 100) {
      status = "NOT CHARGING: charge finished";

      shouldCharge = false;
      voltageHysteresisActivated = true;
    }
    // neer full, prepare to stop (after a fixed time)
    // this handles the battery voltage equalizer process
    else if (batteryLevel >= 95) {
      // stop if the battery voltage did not change for a time
      if (timestamp - lastBatteryRead > 60 * 1000 * timeoutMin) {
        status = "NOT CHARGING: charge finished (bat not full)";

        shouldCharge = false;
        voltageHysteresisActivated = true;
      }
    } else if (timestamp - lastBatteryRead > 60 * 1000 * safeTimeoutMin) {
      status = "NOT CHARGING: charge timeout: battery level stuck";

      shouldCharge = false;

      // battery level high and stable: stop charge
      constexpr uint32_t timeoutRestartMin = 60;
      if (timestamp - lastBatteryRead >
          60 * 1000 * (safeTimeoutMin + timeoutRestartMin)) {
        // reset charge after some time
        voltageHysteresisActivated = false;
        lastBatteryRead = 0;
        lastChargeValue = 0;
      }
    }
  }

  // base test: is the voltage register ok
  if (BQ25703Areg.maxChargeVoltage.get() !=
      uint16_t(batteryMaxVoltage * 1000)) {
    status = "NOT CHARGING: register error " +
             String(BQ25703Areg.maxChargeVoltage.get()) +
             "!=" + String(uint16_t(batteryMaxVoltage * 1000));

    shouldCharge = false;
  }

  // a charger command failed, interrupt charge
  if (charger.isFlagRaised) {
    status = "NOT CHARGING: flag read/write register failed";
    shouldCharge = false;
  }

  // temperature too high, stop charge
  if ((AlertManager.current() & Alerts::TEMP_CRITICAL) != 0x00) {
    status = "NOT CHARGING: temperature critical";

    shouldCharge = false;
  }

  // no power on VBUS, no charge
  if (!PD_UFP.is_vbus_ok()) {
    status = "NOT CHARGING: vbus level not ok";

    shouldCharge = false;
  }
  // the charge ok signal is off
  if (!is_powered_on()) {
    status = "NOT CHARGING: charger ok signal is off";

    shouldCharge = false;
  }
  // else: normal voltages

  static uint32_t startChargeEnabledTime = 0;
  // should not charge battery, or charge is not enabled
  if (!shouldCharge) {
    startChargeEnabledTime = 0;
    isCharging_s = false;
    isChargeEnabled = false;
    isPowerCreepingInProcess_s = false;

    // stop charge already invoked ?
    if (not isChargeResetted) {
      disable_charge();
      isChargeResetted = true;
    }

    // continue to run pd process
    PD_UFP.run();

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

    // read and clear all
    read_alerts();

    //  reset pd negociation
    PD_UFP.set_power_option(PD_POWER_OPTION_MAX_20V);

    // Set the watchdog timer to have a short timeout
    BQ25703Areg.chargeOption0.set_WDTMR_ADJ(1);  // timeout 5 seconds
    charger.writeRegEx(BQ25703Areg.chargeOption0);

    // enable all ADCs
    setChargerADC(true);

    startChargeEnabledTime = millis();
    isChargeEnabled = true;
  }

  //  run pd negociation
  PD_UFP.run();

  isCharging_s = true;

  const float batteryVoltage = battery::get_battery_voltage();
  // safe check (out of bounds)
  if (batteryVoltage <= 0) {
    status = "ERROR: battery voltage read returned wrong values";
    // still return true, this error should correct itself
    return true;
  }

  // default usb voltage/current
  const uint16_t standardUsableCurrent_mA =
      (vbusBasePower * chargerEfficiency) / batteryVoltage;

  uint16_t chargeCurrent_mA = standardUsableCurrent_mA;

  // if we are connected to a PD charger
  if (PD_UFP.is_USB_PD_available()) {
    // the power was negociated & is available on vbus
    if (can_use_max_power()) {
      status = "CHARGING: USB PD power negociated";

      // get_voltage returns values in steps of 50mV (convert to volts)
      const float negociationVoltage = (PD_UFP.get_voltage() * 50) / 1000.0;
      // get_current returns values in steps of 10mA
      const uint16_t negociationCurrent = PD_UFP.get_current() * 10;
      const uint32_t usablePower =
          (negociationVoltage * negociationCurrent) * chargerEfficiency;

      chargeCurrent_mA = usablePower / batteryVoltage;
    } else {
      status = "CHARGING: waiting for USB PD power...";
    }
  }
  // some power may be available, but some charge cable
  // may provide a wrong information, so the charge current should
  // be raised slowly, until the voltage on vbus starts to decrease
  else if (PD_UFP.is_USB_power_available()) {
    // get_current returns values in steps of 10mA
    const uint16_t availableCurrent = PD_UFP.get_current() * 10;
    chargeCurrent_mA =
        regulate_vbus_with_power_creep(batteryVoltage, availableCurrent);
    status = "CHARGING: charging with CC power detection";
  } else {
    // just charge at default voltage/current, with power creep
    chargeCurrent_mA = regulate_vbus_with_power_creep(batteryVoltage, 1000);
    status = "CHARGING: slow charging mode with power creep";
  }
  // limit to max charging current
  chargeCurrent_mA = min(chargeCurrent_mA, batteryMaxChargeCurrent_mA);

  // safety checks (detect bad code above !)
  if (chargeCurrent_mA < standardUsableCurrent_mA) {
    status = "ERROR: charging current less than minimum standard current";
    return false;
  }
  if (chargeCurrent_mA > batteryMaxChargeCurrent_mA) {
    status =
        "ERROR: charging current more than maximum allowed current for "
        "batteries";
    return false;
  }

  // enable component if needed
  enable_charger(false);
  // update the charge current
  BQ25703Areg.chargeCurrent.set(chargeCurrent_mA);

  // flag to signal that the charge is started
  isChargeResetted = false;

  return true;
}

void disable_charge(const bool force) {
  // reset the pd negociation
  PD_UFP.reset();
  PD_UFP.set_power_option(PD_POWER_OPTION_MAX_5V);

  disable_charger(force);

  setChargerADC(false);

  BQ25703Areg.chargeCurrent.set(0);
}

String charge_status() { return status; }

uint16_t get_vbus_voltage_mV() { return PD_UFP.get_vbus_voltage(); }

uint16_t get_charge_current_mA() {
  static uint16_t lastValue = 0;
  static uint32_t lastCall = 0;
  const uint32_t time = millis();
  // only update every seconds
  if (lastCall == 0 or time - lastCall > 1000) {
    lastCall = time;
    lastValue = BQ25703Areg.chargeCurrent.get();
  }
  return lastValue;
}

bool is_slow_charging() {
  return is_charging() and
         // this is just to have a cleaner charger animation during power creep
         not isPowerCreepingInProcess_s and
         // slow charging is defined as the low end of the max charge current
         get_charge_current_mA() < batteryMaxChargeCurrent_mA * 0.25;
}

}  // namespace charger
