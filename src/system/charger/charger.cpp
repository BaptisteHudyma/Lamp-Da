#include "charger.h"

#include <cstdint>

#include "Arduino.h"
#include "BQ25703A/charging_ic.h"
#include "power_source.h"
#include "src/system/alerts.h"
#include "src/system/physical/battery.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace charger {

static bool isBatteryFullLatched_s = false;

// used to stop the charge on user demand (debug behavior)
static bool enableCharge_s = true;

struct BatteryStatus_t
{
  // is battery detected
  bool isPresent = 0;
  // current voltage, in millivolts
  uint16_t voltage_mV = 0;
  // current usage, in milliamps
  // positive is charging, negative is discharging
  int16_t current_mA = 0;

  // minimum & maximum voltage
  uint16_t minVoltage_mV = 0;
  uint16_t maxVoltage_mV = 0;
};
static BatteryStatus_t batteryStatus;

static Charger_t charger;

// check all charge conditions, and return true if the charge should be enabled
bool should_charge()
{
  // user disabled the charge process
  if (not enableCharge_s)
    return false;

  // temperature too high, stop charge
  if ((AlertManager.current() & Alerts::TEMP_CRITICAL) != 0x00)
  {
    return false;
  }

  // no power on VBUS, no charge
  if (not powerSource::is_power_available())
  {
    isBatteryFullLatched_s = false;
    return false;
  }

  // no valid measurments from system yet
  const auto& measurments = BQ25703A::get_measurments();
  if (not measurments.is_measurment_valid())
  {
    return false;
  }
  // get the battery
  const auto& battery = BQ25703A::get_battery();

  // battery not connected
  if (not battery.isPresent)
  {
    isBatteryFullLatched_s = false;
    return false;
  }

  const uint16_t batteryPercent = utils::get_battery_level(utils::get_battery_level_percent(battery.voltage_mV));

  // battery charge is latched on
  if (isBatteryFullLatched_s)
  {
    // deactivate latch below a certain level
    if (batteryPercent < 9500)
    {
      isBatteryFullLatched_s = false;
    }
    return false;
  }

  static uint32_t batteryFullDeglitchTime = 0;

  // check battery full status
  if (isBatteryFullLatched_s or batteryPercent >= 10000)
  {
    const uint32_t time = millis();

    if (batteryFullDeglitchTime == 0)
    {
      batteryFullDeglitchTime = time;
    }
    // the battery needs to stay == 100 for 5 seconds
    else if (time - batteryFullDeglitchTime > 5000)
    {
      batteryFullDeglitchTime = 0;
      // charge done, latch battery level
      isBatteryFullLatched_s = true;
      return false;
    }
  }
  else
  {
    batteryFullDeglitchTime = 0;
  }

  // all conditions passed
  return true;
}

bool is_status_error()
{
  return charger.status == Charger_t::ChargerStatus_t::UNINITIALIZED or
         charger.status == Charger_t::ChargerStatus_t::ERROR_HARDWARE or
         charger.status == Charger_t::ChargerStatus_t::ERROR_SOFTWARE or
         charger.status == Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING;
}

// update the charger status state
void update_state_status()
{
  // check the charger ic first
  switch (BQ25703A::get_status())
  {
    case BQ25703A::Status_t::UNINITIALIZED:
    case BQ25703A::Status_t::ERROR:
      { // locked in a broken state
        // TODO: shutdown ? alert ?
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
        break;
      }
    case BQ25703A::Status_t::ERROR_COMPONENT:
      {
        // broken charger ic
        // TODO: shutdown ? alert ?
        charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
        break;
      }
    case BQ25703A::Status_t::ERROR_HAS_FAULTS:
      {
        // hopefully temporary
        BQ25703A::try_clear_faults();
        // TODO: add a count on the fault clearing
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
        break;
      }
    default:
      {
        // then check the charge status
        switch (BQ25703A::get_charge_status())
        {
          case BQ25703A::ChargeStatus_t::OFF:
            {
              if (isBatteryFullLatched_s)
              {
                charger.status = Charger_t::ChargerStatus_t::CHARGE_FINISHED;
              }
              else if (not BQ25703A::get_battery().isPresent)
              {
                charger.status = Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING;
              }
              else if (powerSource::is_power_available())
                charger.status = Charger_t::ChargerStatus_t::POWER_DETECTED;
              else
                // only return to inactive mode
                charger.status = Charger_t::ChargerStatus_t::INACTIVE;
              break;
            }
          case BQ25703A::ChargeStatus_t::PRECHARGE:
          case BQ25703A::ChargeStatus_t::FASTCHARGE:
            {
              charger.status = Charger_t::ChargerStatus_t::CHARGING;
              break;
            }
          case BQ25703A::ChargeStatus_t::SLOW_CHARGE:
            {
              charger.status = Charger_t::ChargerStatus_t::SLOW_CHARGING;
              break;
            }
          default:
            {
              charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
              break;
            }
        }
        break;
      }
  }
}

// update the charger state
void update_state()
{
  const auto& measurements = BQ25703A::get_measurments();
  if (measurements.is_measurment_valid())
  {
    charger.chargeCurrent_mA = measurements.batChargeCurrent_mA;
    charger.inputCurrent_mA = measurements.vbus_mA;
    charger.vbus_mV = measurements.vbus_mV;

    const auto& battery = BQ25703A::get_battery();
    charger.batteryCurrent_mA = battery.current_mA;
    charger.batteryVoltage_mV = battery.voltage_mV;

    charger.areMeasuresOk = true;
  }
  else
  {
    charger.areMeasuresOk = false;
  }

  update_state_status();
}

/**
 *
 *  HEADER DEFINITIONS
 *
 * */

void setup()
{
  powerSource::setup();

  // start with default parameters
  const bool isChargerEnabled =
          BQ25703A::enable(batteryMinVoltageSafe_mV, batteryMaxVoltage_mV, 4000, batteryMaxChargeCurrent_mA, false);
  if (not isChargerEnabled)
  {
    const auto chargerStatus = BQ25703A::get_status();
    if (chargerStatus == BQ25703A::Status_t::ERROR_COMPONENT)
    {
      // hardware error on the charger part
      charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
    }
    else
    {
      // soft error, still very very bad at startup
      charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
    }

    // abort init
    return;
  }
  // else: init is ok

  charger.status = Charger_t::ChargerStatus_t::INACTIVE;
}

void loop()
{
  // run charger loop
  const bool isChargerOk = (digitalRead(CHARGE_OK) == HIGH);
  BQ25703A::loop(isChargerOk);

  // run power source loop
  powerSource::loop();

  // update the charger state
  update_state();

  // fast fail in case of errors
  if (is_status_error())
  {
    AlertManager.raise_alert(Alerts::HARDWARE_ALERT);
    BQ25703A::enable_charge(false);
    // do NOT run charge functions
    return;
  }
  else
  {
    AlertManager.clear_alert(Alerts::HARDWARE_ALERT);
  }

  // if needed, enable charge
  if (isChargerOk and should_charge())
  {
    BQ25703A::set_input_current_limit(
            // set the max input current for this source
            powerSource::get_max_input_current(),
            // use ICO to find max power we can use with this charger
            not powerSource::is_usb_power_delivery());
    // enable charge
    BQ25703A::enable_charge(true);
  }
  else
  {
    // disable current
    BQ25703A::set_input_current_limit(0, false);
    // disable charge
    BQ25703A::enable_charge(false);
  }
}

void shutdown()
{
  // shutdown charger component
  BQ25703A::shutdown();
}

void set_enable_charge(const bool shouldCharge) { enableCharge_s = shouldCharge; }

bool is_vbus_powered() { return powerSource::is_power_available(); }

bool is_vbus_signal_detected() { return powerSource::is_powered_with_vbus(); }

Charger_t get_state() { return charger; }

bool Charger_t::is_charging() const
{
  return status == ChargerStatus_t::POWER_DETECTED or status == ChargerStatus_t::SLOW_CHARGING or
         status == ChargerStatus_t::CHARGING;
}

String Charger_t::get_status_str() const
{
  switch (status)
  {
    case ChargerStatus_t::UNINITIALIZED:
      return String("UNINITIALIZED");
      break;
    case ChargerStatus_t::INACTIVE:
      return String("INACTIVE");
      break;
    case ChargerStatus_t::POWER_DETECTED:
      return String("POWER_DETECTED");
      break;
    case ChargerStatus_t::SLOW_CHARGING:
      return String("SLOW_CHARGING");
      break;
    case ChargerStatus_t::CHARGING:
      return String("CHARGING");
      break;
    case ChargerStatus_t::CHARGE_FINISHED:
      return String("CHARGE_FINISHED");
      break;
    case ChargerStatus_t::ERROR_BATTERY_MISSING:
      return String("ERROR_BATTERY_MISSING");
      break;
    case ChargerStatus_t::ERROR_SOFTWARE:
      return String("ERROR_SOFTWARE");
      break;
    case ChargerStatus_t::ERROR_HARDWARE:
      return String("ERROR_HARDWARE");
      break;
    default:
      return String("unhandled state");
      break;
  }
}

} // namespace charger