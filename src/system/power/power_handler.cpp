#include "power_handler.h"

#include "src/system/utils/state_machine.h"

#include "src/system/logic/alerts.h"

#include "src/system/physical/battery.h"

#include "src/system/platform/print.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/registers.h"

#include "PDlib/power_delivery.h"
#include "balancer.h"
#include "charger.h"
#include "power_gates.h"

#include <cstdint>
#include <cassert>
#include <stdint.h>

namespace power {

bool _isShutdownCompleted = false;

static constexpr uint32_t clearPowerRailMinDelay_ms = 10;
static constexpr uint32_t clearPowerRailFailureDelay_ms = 1000;
static constexpr uint32_t otgNoUseTimeToDisconnect_ms = 1000;
// timeout in external battery mode
static constexpr uint8_t otgTurnOffTimeMinutes = 5;
static constexpr uint32_t otgNoUseExtBatTimeToDisconnect_ms = 1000 * 60 * otgTurnOffTimeMinutes;

static_assert(clearPowerRailMinDelay_ms < clearPowerRailFailureDelay_ms,
              "clear power rail min activation is less than min unlock delay");

// true voltage/current output limits
static uint16_t _outputVoltage_mV = 0;
static uint16_t _outputCurrent_mA = 0;

// temporary voltage/current limits
static uint16_t _temporaryOutputVoltage_mV = 0;
static uint16_t _temporaryOutputCurrent_mA = 0;
static uint32_t _temporaryOutputTimeOut = 0;

static bool _isChargeEnabled = false;
static std::string _errorStr = "";
static bool _hasAutoSwitchedToOTG = false;

std::string get_error_string()
{
  if (_errorStr.empty())
    return "x";
  return _errorStr;
}
void set_error_state_message(const std::string& errorMsg)
{
  if (_errorStr.empty())
  {
    _errorStr = errorMsg;
  }
}

// Define the state for the main prog state machine
typedef enum
{
  // idle, default mode
  IDLE,

  // prepare any power operation, by wlosing all path and discharging the rails
  CLEAR_POWER_RAILS,

  // Charging
  CHARGING_MODE,

  // output voltage on
  OUTPUT_VOLTAGE_MODE,

  // OnTheGo
  OTG_MODE,

  // effectuate the shutdown
  SHUTDOWN,

  // Should never happen, default state
  ERROR,
} PowerStates;
const char* PowerStatesStr[] = {
        "IDLE",
        "CLEAR_POWER_RAILS",
        "CHARGING_MODE",
        "OUTPUT_VOLTAGE_MODE",
        "OTG_MODE",
        "SHUTDOWN",
        "ERROR",
};

static bool s_isOutputModeReady = false;
bool is_output_mode_ready() { return s_isOutputModeReady; }

namespace __private {

// main state machine (start in error to force user to init)
StateMachine<PowerStates> powerMachine(PowerStates::ERROR);

DigitalPin dischargeVbus(DigitalPin::GPIO::Output_DischargeVbus);
DigitalPin vbusDirection(DigitalPin::GPIO::Output_VbusDirection);
DigitalPin fastRoleSwap(DigitalPin::GPIO::Output_VbusFastRoleSwap);

// if high, signal an USB fault
DigitalPin usbFault(DigitalPin::GPIO::Signal_UsbProtectionFault);
} // namespace __private

uint32_t get_vbus_rail_voltage() { return powerDelivery::get_vbus_voltage(); }

uint32_t get_power_rail_voltage()
{
  const auto& state = charger::get_state();
  if (state.areMeasuresOk)
  {
    return state.powerRail_mV;
  }
  else
  {
    // temp value, will lock the gates until measures are good
    return UINT32_MAX;
  }
}

void set_otg_parameters(uint16_t voltage_mV, uint16_t current_mA)
{
  static uint16_t lastOtgVoltage_mV = 0;
  static uint16_t lastOtgCurrent_mA = 0;
  if (lastOtgVoltage_mV != voltage_mV || lastOtgCurrent_mA != current_mA)
  {
    lastOtgVoltage_mV = voltage_mV;
    lastOtgCurrent_mA = current_mA;

    // ramp up output voltage
    charger::control_OTG(voltage_mV, current_mA);
  }
}

void handle_clear_power_rails()
{
  s_isOutputModeReady = false;

  powerDelivery::suspend_pd_state_machine();

  // disable all gates
  powergates::disable_gates();
  // prevent reverse current flow
  __private::vbusDirection.set_high(false);
  __private::fastRoleSwap.set_high(false);

  // disable charge & balancing if needed
  charger::set_enable_charge(false);
  balancer::enable_balancing(false);
  // disable eventual OTG
  powerDelivery::allow_otg(false);

  // disable OTG if needed
  set_otg_parameters(0, 0);

  // wait at least a little bit
  if (time_ms() - __private::powerMachine.get_state_raised_time() >= clearPowerRailMinDelay_ms and
      // power rail below min measurment voltage
      get_power_rail_voltage() <= 3200)
  {
    __private::dischargeVbus.set_high(false);
    // all is clear, skip to next step
    __private::powerMachine.skip_timeout();
  }
  else // discharge power rail
  {
    __private::dischargeVbus.set_high(true);

    // timeout after which we go to error ?
    if (time_ms() - __private::powerMachine.get_state_raised_time() >= clearPowerRailFailureDelay_ms)
    {
      set_error_state_message("CLEAR_POWER_RAILS: discharge VBUS failed");
      __private::powerMachine.set_state(PowerStates::ERROR);
    }
  }
}

// keep track of current consumption
static uint32_t timeSinceOTGNoCurrentUse;
static uint32_t timeSinceOTGCurrentUse;

void handle_charging_mode()
{
  // resume PD state machine
  powerDelivery::resume_pd_state_machine();
  // enable vbus path
  powergates::enable_vbus_gate();

  // OTG requested, switch to OTG mode
  if (powerDelivery::is_switching_to_otg() and battery::is_battery_usable_as_power_source())
  {
    // disable balancing
    balancer::enable_balancing(false);
    timeSinceOTGNoCurrentUse = time_ms();
    // start otg
    _hasAutoSwitchedToOTG = true;
    __private::powerMachine.set_state(PowerStates::OTG_MODE);
    return;
  }

  if (powergates::is_vbus_gate_enabled())
  {
    charger::set_enable_charge(_isChargeEnabled);
    // balance while we have power on vbus
    balancer::enable_balancing(true);
  }

  // allow OTG in charge mode only
  powerDelivery::allow_otg(true);

  // charge OR idle and do nothing (end of charge)
}

void handle_output_voltage_mode()
{
  static constexpr float voltageGateLowerMultiplier = 0.9f;
  static constexpr float voltageGateHigherMultiplier = 1.1f;

  // never run PD in output mode !
  powerDelivery::suspend_pd_state_machine();

  const uint32_t vbusVoltage = get_power_rail_voltage();

  bool isVbusVoltageOk = false;
  // if we are in temporary output mode, use temporary limits
  if (time_ms() < _temporaryOutputTimeOut)
  {
    set_otg_parameters(_temporaryOutputVoltage_mV, _temporaryOutputCurrent_mA);
    isVbusVoltageOk = (vbusVoltage >= _temporaryOutputVoltage_mV * voltageGateLowerMultiplier and
                       vbusVoltage <= _temporaryOutputVoltage_mV * voltageGateHigherMultiplier);
  }
  else
  {
    _temporaryOutputTimeOut = 0;
    set_otg_parameters(_outputVoltage_mV, _outputCurrent_mA);
    isVbusVoltageOk = (vbusVoltage >= _outputVoltage_mV * voltageGateLowerMultiplier and
                       vbusVoltage <= _outputVoltage_mV * voltageGateHigherMultiplier);
  }

  // enable power gate when voltage matches expected voltage
  if (isVbusVoltageOk)
  {
    // enable power gate
    powergates::enable_power_gate();
    s_isOutputModeReady = true;
  }
  else
  {
    s_isOutputModeReady = false;
    powergates::disable_gates();
  }
}

void handle_otg_mode()
{
  bool otgNoActivity = false;
  const bool isAutoOTGMode = _hasAutoSwitchedToOTG;

  static uint32_t voltageHighRaisedTime = UINT32_MAX;
  // control vbus rail voltage
  const uint32_t vbusVoltage_mv = get_vbus_rail_voltage();

  // shutdown OTG if no current consumption for X seconds
  const auto& state = charger::get_state();
  // no current since a timing
  if (state.inputCurrent_mA <= 10)
  {
    timeSinceOTGCurrentUse = time_ms();

    if (vbusVoltage_mv > 5500 and voltageHighRaisedTime == UINT32_MAX)
    {
      voltageHighRaisedTime = time_ms();
    }

    // if no current use since a timing, stop otg
    const uint32_t otgNonuseTimeout = isAutoOTGMode ? otgNoUseTimeToDisconnect_ms : otgNoUseExtBatTimeToDisconnect_ms;
    if (time_ms() - timeSinceOTGNoCurrentUse > otgNonuseTimeout)
    {
      otgNoActivity = true;
      lampda_print("no OTG activity, shutdown");
    }

    // voltage negociated but not used
    if (voltageHighRaisedTime != UINT32_MAX and time_ms() - voltageHighRaisedTime > 1000)
    {
      otgNoActivity = true;
      lampda_print("no OTG activity and voltage high, shutdown");
    }
  }
  else
  {
    // enable auto mode when power has been used for a time
    if ((not _hasAutoSwitchedToOTG) and time_ms() - timeSinceOTGCurrentUse >= 1000)
    {
      _hasAutoSwitchedToOTG = true;
    }
    if (vbusVoltage_mv > 5500)
    {
      voltageHighRaisedTime = time_ms();
    }
    timeSinceOTGNoCurrentUse = time_ms();
  }

  // if we just switched manually, powerDelivery amy nnot have followed yet
  const bool canUseOtg = true; //(not isAutoOTGMode) or powerDelivery::is_switching_to_otg();

  // end of OTG, switch to charger
  if (otgNoActivity or not canUseOtg or
      // blocking alerts for OTG use
      not battery::is_battery_usable_as_power_source() or not alerts::manager.can_use_usb_port())
  {
    // reset pd machine
    powerDelivery::force_set_to_source_mode(0);
    powerDelivery::suspend_pd_state_machine();
    powerDelivery::resume_pd_state_machine();

    // temporary suspend
    powerDelivery::allow_otg(false);
    set_otg_parameters(0, 0);

    // no need for power gate, we are the one to push current
    __private::powerMachine.set_state(PowerStates::CHARGING_MODE);
    return;
  }
  else
  {
    // resume PD state machine
    powerDelivery::allow_otg(true);
    powerDelivery::resume_pd_state_machine();

    balancer::enable_balancing(false);
    charger::set_enable_charge(false);
  }

  if (not isAutoOTGMode)
  {
    static bool isInOtgModeForce = false;
    if (not isInOtgModeForce)
    {
      powerDelivery::force_set_to_source_mode(1);
      isInOtgModeForce = true;
    }
  }

  static const auto& defaultOTG = powerDelivery::OTGParameters::get_default();
  // requested by system
  auto requestedOtg = powerDelivery::get_otg_parameters();
  requestedOtg.requestedVoltage_mV = max(requestedOtg.requestedVoltage_mV, defaultOTG.requestedVoltage_mV);
  requestedOtg.requestedCurrent_mA = max(requestedOtg.requestedCurrent_mA, defaultOTG.requestedCurrent_mA);
  // should never be true
  if (not requestedOtg.is_otg_requested())
    return;

  // ramp up output voltage
  // then unlock the vbus gate
  set_otg_parameters(requestedOtg.requestedVoltage_mV, requestedOtg.requestedCurrent_mA);

  // allow reverse current flow
  __private::vbusDirection.set_high(true);

  // close vbus gate
  powergates::enable_vbus_gate();
}

void handle_shutdown()
{
  // disable all gates
  powergates::disable_gates();
  set_otg_parameters(0, 0);

  powerDelivery::shutdown();

  // shutdown charger component
  charger::shutdown();

  balancer::go_to_sleep();

  _isShutdownCompleted = true;
}

void handle_error_state()
{
  // disable all gates
  powergates::disable_gates();

  // disable OTG
  set_otg_parameters(0, 0);
}

namespace __private {

/// Handle the state changes
void state_machine_behavior()
{
  powerMachine.run();
  // if state changed, display the new state
  if (powerMachine.state_just_changed())
  {
    lampda_print("POWER_S_MACH > switched to state %s", PowerStatesStr[powerMachine.get_state()]);
  }

  switch (powerMachine.get_state())
  {
    // error state
    case PowerStates::ERROR:
      handle_error_state();
      break;
    // called when system starts
    case PowerStates::IDLE:
      // do nothing
      break;
    // called when the power modes will be started soon
    case PowerStates::CLEAR_POWER_RAILS:
      handle_clear_power_rails();
      break;
    // called when the battery will be charged
    case PowerStates::CHARGING_MODE:
      handle_charging_mode();
      break;
    // called when the output will be powered
    case PowerStates::OUTPUT_VOLTAGE_MODE:
      handle_output_voltage_mode();
      break;
    // called when the vbus voltage will be set by a connected sink
    case PowerStates::OTG_MODE:
      handle_otg_mode();
      break;
    // called when system will be powering down
    case PowerStates::SHUTDOWN:
      handle_shutdown();
      break;
    default:
      {
        set_error_state_message("default case not handled");
        powerMachine.set_state(PowerStates::ERROR);
      }
      break;
  }
}

void switch_state(const PowerStates newState)
{
  powerMachine.set_state(PowerStates::CLEAR_POWER_RAILS, clearPowerRailFailureDelay_ms * 1.5, newState);
}

bool can_switch_states() { return powerMachine.get_state() != PowerStates::ERROR; }

} // namespace __private

bool is_in_error_state() { return __private::powerMachine.get_state() == PowerStates::ERROR; }

// control parameters
bool go_to_output_mode()
{
  if (__private::can_switch_states())
  {
    powerDelivery::suspend_pd_state_machine();
    powerDelivery::force_set_to_source_mode(0);
    powerDelivery::allow_otg(false);
    set_otg_parameters(0, 0);

    __private::switch_state(PowerStates::OUTPUT_VOLTAGE_MODE);
    return true;
  }
  return false;
}

bool go_to_charger_mode()
{
  // TODO: and other checks
  if (__private::can_switch_states())
  {
    powerDelivery::force_set_to_source_mode(0);
    __private::switch_state(PowerStates::CHARGING_MODE);
    return true;
  }

  return false;
}

bool go_to_otg_mode()
{
  if (__private::can_switch_states() and battery::is_battery_usable_as_power_source() and
      alerts::manager.can_use_usb_port())
  {
    timeSinceOTGNoCurrentUse = time_ms();
    powerDelivery::allow_otg(true);
    _hasAutoSwitchedToOTG = false;
    __private::switch_state(PowerStates::OTG_MODE);
    return true;
  }

  return false;
}

bool go_to_idle()
{
  if (__private::can_switch_states())
  {
    __private::switch_state(PowerStates::IDLE);
    return true;
  }

  return false;
}

bool is_state_shutdown_effected() { return _isShutdownCompleted; }

bool go_to_shutdown()
{
  // no need to check if we can switch case in this case
  __private::powerMachine.set_state(PowerStates::SHUTDOWN);

  handle_shutdown();

  return is_state_shutdown_effected();
}

bool go_to_error()
{
  if (__private::powerMachine.set_state(PowerStates::ERROR))
  {
    set_error_state_message("error raised by caller");
  }
  return true;
}

void set_output_voltage_mv(const uint16_t outputVoltage_mV)
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  // NEVER EVER CHANGE INDEXABLE VOLTAGE
  const bool isInRange = (outputVoltage_mV == 0) or (outputVoltage_mV == stripInputVoltage_mV);
  if (not isInRange)
  {
    assert(false);
  }
#endif
  _temporaryOutputTimeOut = 0;
  _outputVoltage_mV = outputVoltage_mV;
}

void set_output_max_current_mA(const uint16_t outputCurrent_mA)
{
  _temporaryOutputTimeOut = 0;
  _outputCurrent_mA = outputCurrent_mA;
}

void set_temporary_output(const uint16_t outputVoltage_mV, const uint16_t outputCurrent_mA, const uint16_t timeout)
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  // NEVER EVER CHANGE INDEXABLE VOLTAGE
  if (outputVoltage_mV != stripInputVoltage_mV)
  {
    assert(false);
  }
#endif
  // set output timeout
  _temporaryOutputTimeOut = time_ms() + timeout;
  _temporaryOutputVoltage_mV = outputVoltage_mV;
  _temporaryOutputCurrent_mA = outputCurrent_mA;
}

// charge mode
bool enable_charge(const bool enable)
{
  _isChargeEnabled = enable;
  return true;
}

std::string get_state() { return std::string(PowerStatesStr[__private::powerMachine.get_state()]); }

bool is_in_output_mode() { return __private::powerMachine.get_state() == PowerStates::OUTPUT_VOLTAGE_MODE; }
bool is_in_otg_mode() { return __private::powerMachine.get_state() == PowerStates::OTG_MODE; }

static bool isSetup = false;

bool is_setup() { return isSetup; }

void init()
{
  // detect shorts in the USB port
  __private::usbFault.attach_callback(
          []() {
            // usb port alert detected
            alerts::manager.raise(alerts::Type::USB_PORT_SHORT);
          },
          DigitalPin::Interrupt::kFallingEdge);

  // init power gates
  powergates::init();

  // switch without a timing
  __private::powerMachine.set_state(PowerStates::IDLE);

  bool isSuccessful = true;
  std::string errorStr = "";

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
  if (not balancer::init())
  {
    errorStr += "\n\t- Init balancer component failed";
    isSuccessful = false;
  }
#endif

  // charging component, setup first
  const bool chargerSuccess = charger::setup();
  if (!chargerSuccess)
  {
    errorStr += "\n\t- Init charger component failed";
    isSuccessful = false;
  }

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
  // at the very last, power delivery
  const bool pdSuccess = powerDelivery::setup();
  if (!pdSuccess)
  {
    errorStr += "\n\t- Init power delivery component failed";
    isSuccessful = false;
  }
#endif

  if (not isSuccessful)
  {
    __private::powerMachine.set_state(PowerStates::ERROR);
    alerts::manager.raise(alerts::Type::HARDWARE_ALERT);

    set_error_state_message(errorStr);
    // failed initialisation, skip
    return;
  }

  isSetup = true;
}

void power_loop()
{
  // kick power watchdog
  kick_watchdog(POWER_WATCHDOG_ID);

  // fist action, update power gate status
  powergates::loop();

  // run power module state machine
  __private::state_machine_behavior();

  // run the power delivery update loop
  powerDelivery::loop();

  // run the charger loop (all the time)
  charger::loop();

  // run the balancer loop (all the time)
  balancer::loop();

  delay_ms(1);
}

void start_threads()
{
  if (!is_setup())
    return;

  //   use the charging thread !
  start_thread(power_loop, power_taskName, 0, 1024);

  powerDelivery::start_threads();
}

} // namespace power
