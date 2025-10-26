#include "charger.h"

#include <cstdint>

#include "balancer.h"
#include "charging_ic.h"
#include "PDlib/power_delivery.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/statistics_handler.h"

#include "src/system/physical/battery.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/registers.h"

namespace charger {

static bool isBatteryFullLatched_s = false;

// used to stop the charge on user demand (debug behavior)
static bool enableCharge_s = true;

struct BatteryStatus_t
{
  // is battery detected
  bool isPresent = false;
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

bool isOtgEnabled_s = false;

// check all charge conditions, and return true if the charge should be enabled
bool should_charge()
{
  // user disabled the charge process
  if (not enableCharge_s)
    return false;

  // no charge if OTG
  if (isOtgEnabled_s)
    return false;

  // wait for balancer
  const auto& balancerStatus = balancer::get_status();
  if (not balancerStatus.is_valid())
    return false;

  static uint32_t batteryTooHighLatchTime = 0;
  for (uint8_t i = 0; i < batteryCount; ++i)
  {
    // do not charge if a battery voltage goes over the max voltage
    if (balancerStatus.batteryVoltages_mV[i] >= maxLiionVoltage_mV)
    {
      batteryTooHighLatchTime = time_ms();
      return false;
    }
  }
  // latch the status of battery too high for a time, to let the balancer work
  if (batteryTooHighLatchTime > 0 and (time_ms() - batteryTooHighLatchTime) < 20000)
  {
    return false;
  }
  batteryTooHighLatchTime = 0;

  // our power source cannot give power
  if (not powerDelivery::can_use_power())
  {
    return false;
  }

  // blocking alert, stop charge
  if (not battery::can_battery_be_charged() or not alerts::manager.can_use_usb_port())
  {
    return false;
  }

  // no power on VBUS, no charge
  if (not powerDelivery::is_power_available())
  {
    isBatteryFullLatched_s = false;
    return false;
  }

  // no valid measurments from system yet
  const auto& measurments = drivers::get_measurments();
  if (not measurments.is_measurment_valid())
  {
    return false;
  }
  // get the battery
  const auto& battery = drivers::get_battery();

  // battery not connected
  if (not battery.isPresent)
  {
    isBatteryFullLatched_s = false;
    return false;
  }

  // battery charge is latched on
  if (isBatteryFullLatched_s)
  {
    // deactivate latch below a certain level
    if (battery::get_level_safe(battery.voltage_mV) < 9500)
    {
      isBatteryFullLatched_s = false;
    }
    return false;
  }

  // reached end of charge state
  const bool isEndOfCharge =
          drivers::get_charge_current() <= 0 and charger.status == Charger_t::ChargerStatus_t::CHARGING;

  static uint32_t batteryFullDeglitchTime = 0;

  // check battery full status
  if (isBatteryFullLatched_s or isEndOfCharge)
  {
    const uint32_t time = time_ms();

    if (batteryFullDeglitchTime == 0)
    {
      batteryFullDeglitchTime = time;
    }
    // the battery needs to stay == 100 for 5 seconds
    else if (time - batteryFullDeglitchTime > 5 * 1000)
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

// Enable the OTG
void control_OTG(const uint16_t mv, const uint16_t ma)
{
  // disable otg
  if (mv == 0 or ma == 0)
  {
    isOtgEnabled_s = false;
    drivers::disable_OTG();
  }
  else
  {
    // reenable battery charging
    isBatteryFullLatched_s = false;

    isOtgEnabled_s = true;
    drivers::set_OTG_targets(mv, ma);
    drivers::enable_OTG();
  }

  const auto& measurments = drivers::get_measurments();
  if (measurments.vbus_mV < mv - 1000 or measurments.vbus_mV > mv + 1000)
    lampda_print("Wait for voltage climb: current %dmV, target %dmV", measurments.vbus_mV, mv);
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
  static Charger_t::ChargerStatus_t previousStatus = Charger_t::ChargerStatus_t::UNINITIALIZED;
  const drivers::Status_t chargerStatus = drivers::get_status();
  // check the charger ic first
  switch (chargerStatus)
  {
    case drivers::Status_t::UNINITIALIZED:
    case drivers::Status_t::ERROR:
      { // locked in a broken state
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          charger.softwareErrorMessage =
                  "ERROR: charger in UNINITIALIZED/ERROR state : " + drivers::get_software_error_message();
          lampda_print(charger.softwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
        break;
      }
    case drivers::Status_t::ERROR_COMPONENT:
      {
        // broken charger ic
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          charger.hardwareErrorMessage = "ERROR: charger in ERROR_COMPONENT state";
          lampda_print(charger.hardwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
        break;
      }
    case drivers::Status_t::ERROR_HAS_FAULTS:
      {
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          charger.softwareErrorMessage = "ERROR: charger in ERROR_HAS_FAULTS state";
          lampda_print(charger.softwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;

        // hopefully temporary
        drivers::try_clear_faults();
        break;
      }
    default:
      {
        // then check the charge status
        switch (drivers::get_charge_status())
        {
          case drivers::ChargeStatus_t::OFF:
            {
              if (not drivers::get_battery().isPresent)
              {
                charger.status = Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING;
              }
              else if (powerDelivery::is_power_available())
                charger.status = Charger_t::ChargerStatus_t::POWER_DETECTED;
              else
                // only return to inactive mode
                charger.status = Charger_t::ChargerStatus_t::INACTIVE;
              break;
            }
          case drivers::ChargeStatus_t::PRECHARGE:
          case drivers::ChargeStatus_t::FASTCHARGE:
            {
              if (isBatteryFullLatched_s)
                charger.status = Charger_t::ChargerStatus_t::CHARGE_FINISHED;
              else
                charger.status = Charger_t::ChargerStatus_t::CHARGING;
              break;
            }
          case drivers::ChargeStatus_t::SLOW_CHARGE:
            {
              charger.status = Charger_t::ChargerStatus_t::SLOW_CHARGING;
              break;
            }
          default:
            {
              charger.hardwareErrorMessage = "ERROR: charger state reached default state";
              lampda_print(charger.hardwareErrorMessage.c_str());

              charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
              break;
            }
        }
        break;
      }
  }
  // update previous status
  previousStatus = charger.status;
}

// update the charger state
void update_state()
{
  const auto& measurements = drivers::get_measurments();
  if (measurements.is_measurment_valid())
  {
    charger.chargeCurrent_mA = measurements.batChargeCurrent_mA;
    charger.inputCurrent_mA = measurements.vbus_mA;
    charger.powerRail_mV = measurements.vbus_mV;
    charger.isChargeOkSignalHigh = measurements.isChargeOk;

    charger.isInOtg = drivers::is_in_OTG();

    const auto& battery = drivers::get_battery();
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
 */

bool setup()
{
  // start with default parameters
  const bool isChargerEnabled = drivers::enable(batteryMinVoltageSafe_mV,
                                                batteryMaxVoltageSafe_mV,
                                                batteryMaxChargeCurrent_mA,
                                                batteryMaxDischargeCurrent_mA,
                                                false);
  if (not isChargerEnabled)
  {
    const auto chargerStatus = drivers::get_status();
    if (chargerStatus == drivers::Status_t::ERROR_COMPONENT)
    {
      // hardware error on the charger part
      charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
      charger.hardwareErrorMessage = drivers::get_status_detail();
    }
    else
    {
      // soft error, still very very bad at startup
      charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
      charger.softwareErrorMessage = drivers::get_software_error_message();
    }

    // abort init
    return false;
  }
  // else: init is ok

  charger.status = Charger_t::ChargerStatus_t::INACTIVE;
  return true;
}

const DigitalPin chargeOkPin(DigitalPin::GPIO::Input_isChargeOk);

void loop()
{
  // run charger loop
  const bool isChargerOk = chargeOkPin.is_high();
  drivers::loop(isChargerOk);

  // update the charger state
  update_state();

  // fast fail in case of errors
  if (is_status_error())
  {
    drivers::enable_charge(false);

    // allow some start time to prevent wrong error display
    if (time_ms() >= 500)
      alerts::manager.raise(alerts::Type::HARDWARE_ALERT);

    // do NOT run charge functions
    return;
  }
  else
  {
    alerts::manager.clear(alerts::Type::HARDWARE_ALERT);
  }

  // if needed, enable charge
  if (isChargerOk and should_charge())
  {
    statistics::signal_battery_charging_on();

    drivers::set_input_current_limit(
            // set the max input current for this source
            powerDelivery::get_max_input_current(),
            // use ICO to find max power we can use with this charger
            powerDelivery::is_standard_port());
    // enable charge
    drivers::enable_charge(true);
  }
  else
  {
    statistics::signal_battery_charging_off();

    // disable current
    drivers::set_input_current_limit(0, false);
    // disable charge
    drivers::enable_charge(false);
  }
}

void shutdown()
{
  // shutdown charger component
  drivers::shutdown();
}

void set_enable_charge(const bool shouldCharge) { enableCharge_s = shouldCharge; }

bool is_vbus_powered() { return powerDelivery::is_power_available(); }

bool can_use_vbus_power() { return powerDelivery::can_use_power(); }

bool is_vbus_signal_detected() { return is_voltage_detected_on_vbus(); }

Charger_t get_state() { return charger; }

bool Charger_t::is_charging() const
{
  return status == ChargerStatus_t::POWER_DETECTED or status == ChargerStatus_t::SLOW_CHARGING or
         status == ChargerStatus_t::CHARGING;
}

bool Charger_t::is_charge_finished() const { return status == ChargerStatus_t::CHARGE_FINISHED; }

bool Charger_t::is_effectivly_charging() const
{
  return status == ChargerStatus_t::SLOW_CHARGING or status == ChargerStatus_t::CHARGING;
}

std::string Charger_t::get_status_str() const
{
  switch (status)
  {
    case ChargerStatus_t::UNINITIALIZED:
      return std::string("UNINITIALIZED");
      break;
    case ChargerStatus_t::INACTIVE:
      return std::string("INACTIVE");
      break;
    case ChargerStatus_t::POWER_DETECTED:
      return std::string("POWER_DETECTED");
      break;
    case ChargerStatus_t::SLOW_CHARGING:
      return std::string("SLOW_CHARGING");
      break;
    case ChargerStatus_t::CHARGING:
      return std::string("CHARGING");
      break;
    case ChargerStatus_t::CHARGE_FINISHED:
      return std::string("CHARGE_FINISHED");
      break;
    case ChargerStatus_t::ERROR_BATTERY_MISSING:
      return std::string("ERROR_BATTERY_MISSING");
      break;
    case ChargerStatus_t::ERROR_SOFTWARE:
      return std::string("ERROR_SOFTWARE");
      break;
    case ChargerStatus_t::ERROR_HARDWARE:
      return std::string("ERROR_HARDWARE");
      break;
    default:
      return std::string("unhandled state");
      break;
  }
}

} // namespace charger
