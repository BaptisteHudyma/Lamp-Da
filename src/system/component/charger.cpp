#include "charger.h"

#include <cstdint>

#include "src/system/hal/time.h"
#include "src/system/hal/gpio.h"
#include "src/system/hal/mutex.h"
#include "src/system/hal/registers.h"

#include "src/system/bsp/pd/power_delivery.h"
#include "src/system/bsp/balancer.h"
#include "src/system/bsp/charging_ic.h"
#include "src/system/bsp/text_out.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/statistics_handler.h"

#include "src/system/component/battery.h"
#include "src/system/component/charge_policy.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace lampda {
namespace component {
namespace charger {

/// Charge stop rules state (battery full, over voltage)
static policy::ChargeLimiter chargeLimiter_s;

// used to stop the charge on user demand (debug behavior)
static bool enableCharge_s = true;

/**
 * \brief Raw battery status measurments
 */
struct BatteryStatus_t
{
  /// is battery detected
  bool isPresent = false;
  /// current voltage, in millivolts
  uint16_t voltage_mV = 0;
  /// current usage, in milliamps.
  /// positive is charging, negative is discharging
  int16_t current_mA = 0;
};
static BatteryStatus_t batteryStatus;

/// Working charger state: only accessed by the power thread, that runs the charger loop
static Charger_t charger;

/// Copy of the charger state given to the readers of all threads, see get_state().
/// The power thread updates it from charger once its update is complete: only access it under _publishedChargerMutex
static Charger_t publishedCharger;

/// Guards publishedCharger: a reader never sees a partial update (status, measurements and error messages). Not
/// recursive
static hal::mutex::hal_mutex_t _publishedChargerMutex;
/// Initialized at program start: the state can be read before the charger setup
[[maybe_unused]] static const bool _isPublishedChargerMutexInit =
        (hal::mutex::hal_mutex_init(&_publishedChargerMutex), true);

/// Publish the working charger state to the readers
static void publish_state()
{
  hal::mutex::hal_mutex_lock(&_publishedChargerMutex);
  publishedCharger = charger;
  hal::mutex::hal_mutex_unlock(&_publishedChargerMutex);
}

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

  // no valid measurments from system yet
  const auto& measurments = bsp::charger::get_measurments();
  if (not measurments.is_measurment_valid())
  {
    return false;
  }

  // turn on voltage for balancer
  const bool isVoltageEnoughForBalancer = measurments.battery_mV >= 3500;

  policy::Inputs limiterInputs;
  if (isVoltageEnoughForBalancer)
  {
    // wait for balancer
    const auto& balancerStatus = bsp::balancer::get_status();
    if (not balancerStatus.is_valid())
      return false;

    limiterInputs.isBalancerValid = true;
    limiterInputs.pack_mV = balancerStatus.stackVoltage_mV;
    for (uint8_t i = 0; i < physical_constants::batterySeriesCount; ++i)
    {
      if (balancerStatus.batteryVoltages_mV[i] > limiterInputs.maxCell_mV)
        limiterInputs.maxCell_mV = balancerStatus.batteryVoltages_mV[i];
    }
  }

  const auto& battery = bsp::charger::get_battery();
  limiterInputs.level = limiterInputs.isBalancerValid ? component::battery::get_level_safe(limiterInputs.pack_mV) :
                                                        component::battery::get_level_safe(battery.voltage_mV);
  limiterInputs.chargeCurrent_mA = bsp::charger::get_charge_current();
  limiterInputs.isCharging = charger.status == Charger_t::ChargerStatus_t::CHARGING;

  // safety first: over voltage stops the charge, even without power source
  const bool isChargeAllowed = chargeLimiter_s.update(hal::time_ms(), limiterInputs);
  if (chargeLimiter_s.is_over_voltage_stopped())
    return false;

  // our power source cannot give power
  if (not bsp::powerDelivery::can_use_power())
  {
    return false;
  }

  // blocking alert, stop charge
  if (not component::battery::can_battery_be_charged() or not logic::alerts::manager.can_use_usb_port())
  {
    return false;
  }

  // no power on VBUS, no charge
  if (not bsp::powerDelivery::is_power_available())
  {
    chargeLimiter_s.clear_full();
    return false;
  }

  // battery not connected
  if (not battery.isPresent)
  {
    chargeLimiter_s.clear_full();
    return false;
  }

  // battery full, or end of charge reached
  if (not isChargeAllowed)
    return false;

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
    bsp::charger::disable_OTG();
  }
  else
  {
    // reenable battery charging
    chargeLimiter_s.clear_full();

    isOtgEnabled_s = true;
    bsp::charger::set_OTG_targets(mv, ma);
    bsp::charger::enable_OTG();
  }
}

bool is_status_error()
{
  return charger.status == Charger_t::ChargerStatus_t::UNINITIALIZED or
         charger.status == Charger_t::ChargerStatus_t::ERROR_HARDWARE or
         charger.status == Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
}

// update the charger status state
void update_state_status()
{
  static Charger_t::ChargerStatus_t previousStatus = Charger_t::ChargerStatus_t::UNINITIALIZED;
  const bsp::charger::Status_t chargerStatus = bsp::charger::get_status();
  // check the charger ic first
  switch (chargerStatus)
  {
    case bsp::charger::Status_t::UNINITIALIZED:
    case bsp::charger::Status_t::ERROR:
      { // locked in a broken state
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          charger.softwareErrorMessage = etl::string<128>("ERROR: charger in UNINITIALIZED/ERROR state : ")
                                                 .append(bsp::charger::get_software_error_message());
          bsp::lampda_print(charger.softwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
        break;
      }
    case bsp::charger::Status_t::ERROR_COMPONENT:
      {
        // broken charger ic
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          charger.hardwareErrorMessage = "ERROR: charger in ERROR_COMPONENT state";
          bsp::lampda_print(charger.hardwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
        break;
      }
    case bsp::charger::Status_t::ERROR_HAS_FAULTS:
      {
        if (previousStatus != Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          charger.softwareErrorMessage = "ERROR: charger in ERROR_HAS_FAULTS state";
          bsp::lampda_print(charger.softwareErrorMessage.c_str());
        }
        charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;

        // hopefully temporary
        bsp::charger::try_clear_faults();
        break;
      }
    default:
      {
        // then check the charge status
        switch (bsp::charger::get_charge_status())
        {
          case bsp::charger::ChargeStatus_t::OFF:
            {
              if (not bsp::charger::get_battery().isPresent)
              {
                charger.status = Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING;
              }
              else if (bsp::powerDelivery::is_power_available())
                charger.status = Charger_t::ChargerStatus_t::POWER_DETECTED;
              else
                // only return to inactive mode
                charger.status = Charger_t::ChargerStatus_t::INACTIVE;
              break;
            }
          case bsp::charger::ChargeStatus_t::PRECHARGE:
          case bsp::charger::ChargeStatus_t::FASTCHARGE:
            {
              if (chargeLimiter_s.is_full())
                charger.status = Charger_t::ChargerStatus_t::CHARGE_FINISHED;
              else
                charger.status = Charger_t::ChargerStatus_t::CHARGING;
              break;
            }
          case bsp::charger::ChargeStatus_t::SLOW_CHARGE:
            {
              charger.status = Charger_t::ChargerStatus_t::SLOW_CHARGING;
              break;
            }
          default:
            {
              charger.hardwareErrorMessage = "ERROR: charger state reached default state";
              bsp::lampda_print(charger.hardwareErrorMessage.c_str());

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
  const auto& measurements = bsp::charger::get_measurments();
  if (measurements.is_measurment_valid())
  {
    charger.chargeCurrent_mA = measurements.batChargeCurrent_mA;
    charger.inputCurrent_mA = measurements.vbus_mA;
    charger.powerRail_mV = measurements.vbus_mV;
    charger.isChargeOkSignalHigh = measurements.isChargeOk;

    charger.isInOtg = bsp::charger::is_in_OTG();

    const auto& battery = bsp::charger::get_battery();
    charger.batteryCurrent_mA = battery.current_mA;
    charger.batteryVoltage_mV = battery.voltage_mV;

    charger.areMeasuresOk = true;
  }
  else
  {
    charger.areMeasuresOk = false;
  }

  // update charger status
  update_state_status();

  // set update time
  charger.lastUpdateTime_ms = hal::time_ms();

  // the update is complete: give it to the readers
  publish_state();
}

/**
 *
 *  HEADER DEFINITIONS
 *
 */

bool isSetup = false;
bool setup()
{
  if (isSetup)
    return true;

  // start with default parameters
  const bool isChargerEnabled = bsp::charger::enable(physical_constants::batteryMinVoltageSafe_mV,
                                                     physical_constants::batteryMaxVoltageSafe_mV,
                                                     physical_constants::batteryMaxChargeCurrent_mA,
                                                     physical_constants::batteryMaxDischargeCurrent_mA,
                                                     false);
  if (not isChargerEnabled)
  {
    const auto chargerStatus = bsp::charger::get_status();
    if (chargerStatus == bsp::charger::Status_t::ERROR_COMPONENT)
    {
      // hardware error on the charger part
      charger.status = Charger_t::ChargerStatus_t::ERROR_HARDWARE;
      charger.hardwareErrorMessage = bsp::charger::get_status_detail();
    }
    else
    {
      // soft error, still very very bad at startup
      charger.status = Charger_t::ChargerStatus_t::ERROR_SOFTWARE;
      charger.softwareErrorMessage = bsp::charger::get_software_error_message();
    }
    publish_state();

    // abort init
    return false;
  }
  // else: init is ok

  charger.status = Charger_t::ChargerStatus_t::INACTIVE;
  publish_state();
  isSetup = true;
  return true;
}

const hal::gpio::DigitalPin chargeOkPin(hal::gpio::DigitalPin::GPIO::Input_isChargeOk);

void loop()
{
  if (not isSetup)
    return;

  // run charger loop
  const bool isChargerOk = chargeOkPin.is_high();
  bsp::charger::loop(isChargerOk);

  // update the charger state
  update_state();

  // fast fail in case of errors
  static bool isHardwareErrorRaiser = false;
  if (is_status_error())
  {
    bsp::charger::enable_charge(false);

    // allow some start time to prevent wrong error display
    if (hal::time_ms() >= 500)
    {
      logic::alerts::manager.raise(logic::alerts::Type::CHARGER_ERROR);
    }
    // do NOT run charge functions
    return;
  }
  else
  {
    logic::alerts::manager.clear(logic::alerts::Type::CHARGER_ERROR);
  }

  // if needed, enable charge
  if (isChargerOk and should_charge())
  {
    logic::statistics::signal_battery_charging_on();

    bsp::charger::set_input_current_limit(
            // set the max input current for this source
            bsp::powerDelivery::get_max_input_current(),
            // use ICO to find max power we can use with this charger
            bsp::powerDelivery::is_standard_port());
    // enable charge
    bsp::charger::enable_charge(true);
  }
  else
  {
    logic::statistics::signal_battery_charging_off();

    // disable current
    bsp::charger::set_input_current_limit(0, false);
    // disable charge
    bsp::charger::enable_charge(false);
  }
}

void shutdown()
{
  // shutdown charger component
  bsp::charger::shutdown();
}

void set_enable_charge(const bool shouldCharge) { enableCharge_s = shouldCharge; }

bool is_vbus_powered() { return bsp::powerDelivery::is_power_available(); }

bool can_use_vbus_power() { return bsp::powerDelivery::can_use_power(); }

bool is_vbus_signal_detected() { return hal::registers::is_voltage_detected_on_vbus(); }

Charger_t get_state()
{
  hal::mutex::hal_mutex_lock(&_publishedChargerMutex);
  const Charger_t state = publishedCharger;
  hal::mutex::hal_mutex_unlock(&_publishedChargerMutex);
  return state;
}

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

etl::string<32> Charger_t::get_status_str() const
{
  switch (status)
  {
    case ChargerStatus_t::UNINITIALIZED:
      return etl::string<32>("UNINITIALIZED");
      break;
    case ChargerStatus_t::INACTIVE:
      return etl::string<32>("INACTIVE");
      break;
    case ChargerStatus_t::POWER_DETECTED:
      return etl::string<32>("POWER_DETECTED");
      break;
    case ChargerStatus_t::SLOW_CHARGING:
      return etl::string<32>("SLOW_CHARGING");
      break;
    case ChargerStatus_t::CHARGING:
      return etl::string<32>("CHARGING");
      break;
    case ChargerStatus_t::CHARGE_FINISHED:
      return etl::string<32>("CHARGE_FINISHED");
      break;
    case ChargerStatus_t::ERROR_BATTERY_MISSING:
      return etl::string<32>("ERROR_BATTERY_MISSING");
      break;
    case ChargerStatus_t::ERROR_SOFTWARE:
      return etl::string<32>("ERROR_SOFTWARE");
      break;
    case ChargerStatus_t::ERROR_HARDWARE:
      return etl::string<32>("ERROR_HARDWARE");
      break;
    default:
      return etl::string<32>("unhandled state");
      break;
  }
}

} // namespace charger
} // namespace component
} // namespace lampda
