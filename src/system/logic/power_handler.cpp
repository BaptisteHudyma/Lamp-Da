#include "power_handler.h"

#include "src/system/utils/state_machine.h"

#include "src/system/logic/alerts.h"

#include "src/system/physical/battery.h"

#include "src/system/platform/print.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/registers.h"

#include "src/system/power/balancer.h"
#include "src/system/power/charger.h"
#include "src/system/power/PDlib/power_delivery.h"
#include "src/system/power/power_gates.h"

#include <cstdint>
#include <cassert>
#include <stdint.h>

namespace lampda {
namespace logic {
namespace power {

// references
void power_loop();
// \references

/// Set to true if the shutdown process is finished cleanly.
bool _isShutdownCompleted = false;

/// Minimum allowed power rail clear delay.
static constexpr uint32_t clearPowerRailMinDelay_ms = 10;
/// Minimum power rail clear delay after which a failure is signaled.
/// In some rare case the rail can take up to 2 seconds to clear itself.
static constexpr uint32_t clearPowerRailFailureDelay_ms = 3000;
/// Delay after which the OTG_MODE will auto disable if a disconnection is detected.
static constexpr uint32_t otgNoUseTimeToDisconnect_ms = 1000;
/// Delay after which the OTG_MODE will auto disable if not used.
static constexpr uint8_t otgTurnOffTimeMinutes = 5;
/// Delay after which the OTG_MODE will auto disable if not used, in milliseconds.
static constexpr uint32_t otgNoUseExtBatTimeToDisconnect_ms = 1000 * 60 * otgTurnOffTimeMinutes;
/// Delay after which the STARTUP state will auto switch to error.
static constexpr uint32_t startupFailTimeout_ms = 3000;

static_assert(clearPowerRailMinDelay_ms < clearPowerRailFailureDelay_ms,
              "clear power rail min activation is less than min unlock delay");

/// true voltage output
static uint16_t _outputVoltage_mV = 0;
/// true current output limit
static uint16_t _outputCurrent_mA = 0;

/// temporary voltage
static uint16_t _temporaryOutputVoltage_mV = 0;
/// temporary current limits
static uint16_t _temporaryOutputCurrent_mA = 0;
/// temporary output timeout time
static uint32_t _temporaryOutputTimeOut = 0;

/// True if battery charging is allowed
static bool _isChargeEnabled = false;

/// Store the state error description
static std::string _errorStr = "";

/// True if the state changed to OTG_MODE on it's own (via USB-PD requests).
static bool _hasAutoSwitchedToOTG = false;

/// in this mode, the battery is too low to power the components.
/// They will be powered through the vbus gate.
static bool _isInBatteryRecoveryMode = false;
// This flag is never cleared and allows use to check this case
static bool _wasStartedInBatteryRecoveryMode = false;

/// Return the error string if set, or "x"
std::string get_error_string()
{
  if (_errorStr.empty())
    return "x";
  return _errorStr;
}

/// Set the error message, if not already set.
void set_error_state_message(const std::string& errorMsg)
{
  if (_errorStr.empty())
  {
    _errorStr = errorMsg;
  }
}

/// Define the state for the power state machine
using PowerStates = enum class power_states_t
{
  /// Start phase, initialization and checks.
  STARTUP,

  /// Idle, default mode. No power output.
  IDLE,

  /// Prepare any power operation, by closing all power path and discharging the power rails.
  CLEAR_POWER_RAILS,

  /// Charge the batteries. Send power from the USB port to the batteries.
  CHARGING_MODE,

  /// Output voltage on. Send voltage from the batteries to the board Output.
  OUTPUT_VOLTAGE_MODE,

  /// External battery mode (OnTheGo). Send power from the battery to the USB port.
  OTG_MODE,

  /// Shutdown the system and components gracefully.
  SHUTDOWN,

  /// Default safety state. if reached, no power will be sent.
  ERROR,
};

/// Name of the states as strings
const char* const PowerStatesStr[] = {
        "STARTUP",
        "IDLE",
        "CLEAR_POWER_RAILS",
        "CHARGING_MODE",
        "OUTPUT_VOLTAGE_MODE",
        "OTG_MODE",
        "SHUTDOWN",
        "ERROR",
};

/// True if the OUTPUT_MODE state is ready, ie if the mode is active, the power is as requested and the output gate is
/// enabled.
static bool s_isOutputModeReady = false;
bool is_output_mode_ready() { return s_isOutputModeReady; }

/// Internal implementations
namespace __private {

// main state machine (start with timeout, go to error on timeout)
utils::StateMachine<PowerStates> powerMachine(PowerStates::STARTUP, startupFailTimeout_ms, PowerStates::ERROR);

const platform::gpio::DigitalPin dischargeVbus(platform::gpio::DigitalPin::GPIO::Output_DischargeVbus);
const platform::gpio::DigitalPin vbusDirection(platform::gpio::DigitalPin::GPIO::Output_VbusDirection);
const platform::gpio::DigitalPin fastRoleSwap(platform::gpio::DigitalPin::GPIO::Output_VbusFastRoleSwap);

// if high, signal an USB fault
const platform::gpio::DigitalPin usbFault(platform::gpio::DigitalPin::GPIO::Signal_UsbProtectionFault);
// if high, signal a vbus gate fault
const platform::gpio::DigitalPin vbusFault(platform::gpio::DigitalPin::GPIO::Signal_VbusGateFault);
} // namespace __private

uint32_t get_vbus_rail_voltage() { return ::lampda::power::powerDelivery::get_vbus_voltage(); }

uint32_t get_power_rail_voltage()
{
  const auto& state = ::lampda::power::charger::get_state();
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
    ::lampda::power::charger::control_OTG(voltage_mV, current_mA);
  }
}

void handle_clear_power_rails()
{
  s_isOutputModeReady = false;
  if (_isInBatteryRecoveryMode)
  {
    // all is clear, skip to next step
    __private::dischargeVbus.set_high(false);
    __private::powerMachine.skip_timeout();
    return;
  }

  ::lampda::power::powerDelivery::suspend_pd_state_machine();

  // disable all gates
  ::lampda::power::powergates::disable_gates();
  // prevent reverse current flow
  __private::vbusDirection.set_high(false);
  __private::fastRoleSwap.set_high(false);
  // discharge power rail
  __private::dischargeVbus.set_high(true);

  // disable charge & balancing if needed
  ::lampda::power::charger::set_enable_charge(false);
  ::lampda::power::balancer::enable_balancing(false);
  // disable eventual OTG
  ::lampda::power::powerDelivery::allow_otg(false);

  // disable OTG if needed
  set_otg_parameters(0, 0);

  // wait at least a little bit
  const auto powerRailVoltage_mv = get_power_rail_voltage();
  if (platform::time_ms() - __private::powerMachine.get_state_raised_time() >= clearPowerRailMinDelay_ms and
      // power rail below min measurment voltage
      (powerRailVoltage_mv <= 3200))
  {
    __private::dischargeVbus.set_high(false);
    // all is clear, skip to next step
    __private::powerMachine.skip_timeout();
    return;
  }
  else // discharge power rail
  {
    __private::dischargeVbus.set_high(true);

    // timeout after which we go to error ?
    if (platform::time_ms() - __private::powerMachine.get_state_raised_time() >= clearPowerRailFailureDelay_ms)
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
  ::lampda::power::powerDelivery::resume_pd_state_machine();
  // enable vbus path
  ::lampda::power::powergates::enable_vbus_gate();

  // OTG requested, switch to OTG mode
  if (::lampda::power::powerDelivery::is_switching_to_otg() and physical::battery::is_battery_usable_as_power_source())
  {
    // disable balancing
    ::lampda::power::balancer::enable_balancing(false);
    timeSinceOTGNoCurrentUse = platform::time_ms();
    // start otg
    _hasAutoSwitchedToOTG = true;
    __private::powerMachine.set_state(PowerStates::OTG_MODE);
    return;
  }

  if (::lampda::power::powergates::is_vbus_gate_enabled())
  {
    ::lampda::power::charger::set_enable_charge(_isChargeEnabled);
    // balance while we have power on vbus
    ::lampda::power::balancer::enable_balancing(true);
  }

  // allow OTG in charge mode only
  ::lampda::power::powerDelivery::allow_otg(true);

  // charge OR idle and do nothing (end of charge)
}

void handle_output_voltage_mode()
{
  static constexpr float voltageGateLowerMultiplier = 0.9f;
  static constexpr float voltageGateHigherMultiplier = 1.1f;

  // never run PD in output mode !
  ::lampda::power::powerDelivery::suspend_pd_state_machine();

  const auto powerRailVoltage_mv = get_power_rail_voltage();

  bool isVoltageOk = false;
  // if we are in temporary output mode, use temporary limits
  if (platform::time_ms() < _temporaryOutputTimeOut)
  {
    set_otg_parameters(_temporaryOutputVoltage_mV, _temporaryOutputCurrent_mA);
    isVoltageOk =
            (powerRailVoltage_mv >= static_cast<uint32_t>(stripInputMinVoltage_mV * voltageGateLowerMultiplier) and
             // do not check the upper bound, this is a special DANGEROUS case
             powerRailVoltage_mv <= static_cast<uint32_t>(_temporaryOutputVoltage_mV * voltageGateHigherMultiplier));
  }
  else
  {
    _temporaryOutputTimeOut = 0;
    set_otg_parameters(_outputVoltage_mV, _outputCurrent_mA);
    isVoltageOk =
            (powerRailVoltage_mv >= static_cast<uint32_t>(stripInputMinVoltage_mV * voltageGateLowerMultiplier) and
             powerRailVoltage_mv <= static_cast<uint32_t>(stripInputMaxVoltage_mV * voltageGateHigherMultiplier));
  }

  // enable power gate when voltage matches expected voltage
  if (isVoltageOk)
  {
    // enable power gate
    ::lampda::power::powergates::enable_power_gate();

    if (not s_isOutputModeReady)
      platform::lampda_print("voltage ready on output gate");
    s_isOutputModeReady = true;
  }
  else
  {
    if (s_isOutputModeReady)
      platform::lampda_print("voltage is not in required range, disabling output. Actual %dmV. Required %dmV",
                             powerRailVoltage_mv,
                             _outputVoltage_mV);
    s_isOutputModeReady = false;

    ::lampda::power::powergates::disable_gates();
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
  const auto& state = ::lampda::power::charger::get_state();
  // no current since a timing
  if (state.inputCurrent_mA <= 10)
  {
    timeSinceOTGCurrentUse = platform::time_ms();

    if (vbusVoltage_mv > 5500 and voltageHighRaisedTime == UINT32_MAX)
    {
      platform::lampda_print("OTG mode: Voltage high registered: %dmv", vbusVoltage_mv);
      voltageHighRaisedTime = platform::time_ms();
    }

    // if no current use since a timing, stop otg
    const uint32_t otgNonuseTimeout = isAutoOTGMode ? otgNoUseTimeToDisconnect_ms : otgNoUseExtBatTimeToDisconnect_ms;
    if (platform::time_ms() - timeSinceOTGNoCurrentUse > otgNonuseTimeout)
    {
      otgNoActivity = true;
      platform::lampda_print("no OTG activity, shutdown");
    }

    // voltage negociated but not used
    if (voltageHighRaisedTime != UINT32_MAX and platform::time_ms() - voltageHighRaisedTime > 1000)
    {
      otgNoActivity = true;
      platform::lampda_print("no OTG activity and voltage high, shutdown");
    }
  }
  else
  {
    // enable auto mode when power has been used for a time
    if ((not _hasAutoSwitchedToOTG) and platform::time_ms() - timeSinceOTGCurrentUse >= 1000)
    {
      _hasAutoSwitchedToOTG = true;
    }
    if (vbusVoltage_mv > 5500)
    {
      voltageHighRaisedTime = platform::time_ms();
    }
    timeSinceOTGNoCurrentUse = platform::time_ms();
  }

  // if we just switched manually, powerDelivery amy nnot have followed yet
  const bool canUseOtg = true; //(not isAutoOTGMode) or powerDelivery::is_switching_to_otg();

  // end of OTG, switch to charger
  if (otgNoActivity or not canUseOtg or
      // blocking alerts for OTG use
      not physical::battery::is_battery_usable_as_power_source() or not logic::alerts::manager.can_use_usb_port())
  {
    // reset pd machine
    ::lampda::power::powerDelivery::force_set_to_source_mode(false);
    ::lampda::power::powerDelivery::suspend_pd_state_machine();
    ::lampda::power::powerDelivery::resume_pd_state_machine();

    // temporary suspend
    ::lampda::power::powerDelivery::allow_otg(false);
    set_otg_parameters(0, 0);

    // no need for power gate, we are the one to push current
    __private::powerMachine.set_state(PowerStates::CHARGING_MODE);
    return;
  }
  else
  {
    // resume PD state machine
    ::lampda::power::powerDelivery::allow_otg(true);
    ::lampda::power::powerDelivery::resume_pd_state_machine();

    ::lampda::power::balancer::enable_balancing(false);
    ::lampda::power::charger::set_enable_charge(false);
  }

  if (not isAutoOTGMode)
  {
    static bool isInOtgModeForce = false;
    if (not isInOtgModeForce)
    {
      ::lampda::power::powerDelivery::force_set_to_source_mode(true);
      isInOtgModeForce = true;
    }
  }

  static const auto& defaultOTG = ::lampda::power::powerDelivery::OTGParameters::get_default();
  // requested by system
  auto requestedOtg = ::lampda::power::powerDelivery::get_otg_parameters();
  requestedOtg.requestedVoltage_mV = max<uint16_t>(requestedOtg.requestedVoltage_mV, defaultOTG.requestedVoltage_mV);
  requestedOtg.requestedCurrent_mA = max<uint16_t>(requestedOtg.requestedCurrent_mA, defaultOTG.requestedCurrent_mA);
  // should never be true
  if (not requestedOtg.is_otg_requested())
    return;

  // ramp up output voltage
  // then unlock the vbus gate
  set_otg_parameters(requestedOtg.requestedVoltage_mV, requestedOtg.requestedCurrent_mA);

  // allow reverse current flow
  __private::vbusDirection.set_high(true);

  // close vbus gate
  ::lampda::power::powergates::enable_vbus_gate();
}

void handle_shutdown()
{
  // disable all gates
  ::lampda::power::powergates::disable_gates();
  set_otg_parameters(0, 0);

  ::lampda::power::powerDelivery::shutdown();

  // shutdown charger component
  ::lampda::power::charger::shutdown();

  ::lampda::power::balancer::go_to_sleep();

  _isShutdownCompleted = true;
}

void handle_startup()
{
  // no user setup (yet ?)
  if (not is_setup())
    return;

  // if this is false, it's a special case where battery voltage is too low to power the components
  // or the components are dead...
  if (platform::i2c::i2c_check_existence(0, platform::i2c::chargeI2cAddress) != 0
  // TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
      || platform::i2c::i2c_check_existence(0, platform::i2c::batteryBalancerI2cAddress) != 0
#endif
  )
  {
    const uint32_t timeSinceStateSwitch = platform::time_ms() - __private::powerMachine.get_state_raised_time();
    const bool isVbusUnpowered =
            timeSinceStateSwitch > 200 and not ::lampda::power::powerDelivery::is_power_available();

    // no vbus, or
    if (timeSinceStateSwitch > startupFailTimeout_ms * 0.3 or isVbusUnpowered)
    {
      logic::alerts::manager.raise(logic::alerts::Type::HARDWARE_ALERT);
      if (isVbusUnpowered)
      {
        set_error_state_message("no i2c response from charger");
      }
      else
      {
        set_error_state_message("no i2c response from charger with power enabled");
      }
      __private::powerMachine.set_state(PowerStates::ERROR);
      _isInBatteryRecoveryMode = false;
      return;
    }

    if (not isVbusUnpowered)
    {
      // Recover strategy :
      // unlock the power gate and check that charger and balancer are detected.
      ::lampda::power::powergates::enable_vbus_gate_DIRECT();
    }
    _isInBatteryRecoveryMode = true;
    _wasStartedInBatteryRecoveryMode = true;

    // hold the state
    return;
  }

  /**
   * Component initialization
   */

  // charging component, setup first
  const bool chargerSuccess = ::lampda::power::charger::setup();
  if (!chargerSuccess)
  {
    set_error_state_message("\n\t- Init charger component failed");
    __private::powerMachine.set_state(PowerStates::ERROR);
    logic::alerts::manager.raise(logic::alerts::Type::HARDWARE_ALERT);
    // failed initialisation, skip
    return;
  }

  // battery in recovery : charger must start a charge cycle to power the balancer
  if (_isInBatteryRecoveryMode)
  {
    ::lampda::power::charger::set_enable_charge(true);

    // wait a bit
    const auto& state = ::lampda::power::charger::get_state();
    // wait for charge
    const bool isCharging = state.areMeasuresOk && state.is_charging();
    if (not isCharging)
      return;
    const uint32_t timeSinceStateSwitch = platform::time_ms() - __private::powerMachine.get_state_raised_time();
    if (timeSinceStateSwitch < startupFailTimeout_ms * 0.8)
      return;

    if (not ::lampda::power::powerDelivery::is_power_available())
    {
      set_error_state_message("\n\t- Battery recovery mode failed");
      return;
    }
    // This delay is to let the time for the balancer to boot.
    platform::delay_ms(250);
  }
  // Final point: We will never pass through here again !

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
  if (not ::lampda::power::balancer::init())
  {
    if (_isInBatteryRecoveryMode)
    {
      set_error_state_message(
              "\n\t- Init balancer component failed in recovery mode: no battery or dead forever battery");
    }
    else
      set_error_state_message("\n\t- Init balancer component failed");

    __private::powerMachine.set_state(PowerStates::ERROR);
    logic::alerts::manager.raise(logic::alerts::Type::HARDWARE_ALERT);
    // failed initialisation, skip
    return;
  }
#endif

  /*
   * All good !
   */

  // switch without a timing
  __private::powerMachine.set_state(PowerStates::IDLE);
}

void handle_error_state()
{
  _isInBatteryRecoveryMode = false;

  // disable all gates
  ::lampda::power::powergates::disable_gates();

  // disable OTG
  set_otg_parameters(0, 0);

  // previous state was startup, and timeout
  if (__private::powerMachine.get_last_state() == PowerStates::STARTUP and
      __private::powerMachine.state_changed_with_timeout())
  {
    set_error_state_message("startup state timeout");
  }
}

namespace __private {

/// Handle the state changes
void state_machine_behavior()
{
  powerMachine.run();
  // if state changed, display the new state
  if (powerMachine.state_just_changed())
  {
    platform::lampda_print("POWER_S_MACH > switched to state %s",
                           PowerStatesStr[static_cast<size_t>(powerMachine.get_state())]);
  }

  switch (powerMachine.get_state())
  {
    case PowerStates::STARTUP:
      handle_startup();
      break;
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
  powerMachine.set_state(
          PowerStates::CLEAR_POWER_RAILS, static_cast<uint32_t>(clearPowerRailFailureDelay_ms * 1.5f), newState);
}

bool can_switch_states() { return powerMachine.get_state() != PowerStates::ERROR; }

} // namespace __private

bool is_in_error_state() { return __private::powerMachine.get_state() == PowerStates::ERROR; }

// control parameters
bool go_to_output_mode()
{
  if (_isInBatteryRecoveryMode)
    return false;

  if (__private::can_switch_states())
  {
    ::lampda::power::powerDelivery::suspend_pd_state_machine();
    ::lampda::power::powerDelivery::force_set_to_source_mode(false);
    ::lampda::power::powerDelivery::allow_otg(false);
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
    ::lampda::power::powerDelivery::force_set_to_source_mode(false);
    __private::switch_state(PowerStates::CHARGING_MODE);
    return true;
  }

  return false;
}

bool go_to_otg_mode()
{
  if (_isInBatteryRecoveryMode)
    return false;

  if (__private::can_switch_states() and physical::battery::is_battery_usable_as_power_source() and
      logic::alerts::manager.can_use_usb_port())
  {
    timeSinceOTGNoCurrentUse = platform::time_ms();
    ::lampda::power::powerDelivery::allow_otg(true);
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

bool go_to_shutdown()
{
  // no need to check if we can switch case in this case
  __private::powerMachine.set_state(PowerStates::SHUTDOWN);

  handle_shutdown();

  return _isShutdownCompleted;
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
  if constexpr (stripInputMinVoltage_mV == stripInputMaxVoltage_mV)
  {
    // NEVER EVER CHANGE fixed output VOLTAGE
    const bool isInRange = (outputVoltage_mV == 0) or (outputVoltage_mV == stripInputMaxVoltage_mV);
    if (not isInRange)
    {
      assert(false);
    }
  }

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
  if constexpr (stripInputMinVoltage_mV == stripInputMaxVoltage_mV)
  {
    // NEVER EVER CHANGE FIXED OUTPUT VOLTAGE
    if (outputVoltage_mV != stripInputMaxVoltage_mV)
    {
      assert(false);
    }
  }

  // set output timeout
  _temporaryOutputTimeOut = platform::time_ms() + timeout;
  _temporaryOutputVoltage_mV = outputVoltage_mV;
  _temporaryOutputCurrent_mA = outputCurrent_mA;
}

// charge mode
bool enable_charge(const bool enable)
{
  _isChargeEnabled = enable;
  return true;
}

std::string get_state()
{
  return std::string(PowerStatesStr[static_cast<size_t>(__private::powerMachine.get_state())]);
}

bool is_in_output_mode() { return __private::powerMachine.get_state() == PowerStates::OUTPUT_VOLTAGE_MODE; }
bool is_in_otg_mode() { return __private::powerMachine.get_state() == PowerStates::OTG_MODE; }
bool was_started_in_battery_recovery() { return _wasStartedInBatteryRecoveryMode; }

static bool isSetup = false;

bool is_setup() { return isSetup; }

bool is_started() { return __private::powerMachine.get_state() != PowerStates::STARTUP; }

void init()
{
  // detect shorts in the USB port
  __private::usbFault.attach_callback(
          []() {
            // usb port alert detected
            logic::alerts::manager.raise(logic::alerts::Type::USB_PORT_SHORT);
          },
          platform::gpio::DigitalPin::Interrupt::kFallingEdge);

  /// TODO: issue #326 handle the VBUS gate fault cleanly
#ifdef LMBD_SIMULATION
  __private::vbusFault.attach_callback(
          []() {
            platform::lampda_print("-----------------");
            platform::lampda_print("VBUS FAULT RAISED");
            platform::lampda_print("-----------------");
          },
          platform::gpio::DigitalPin::Interrupt::kFallingEdge);
#endif

  // init power gates
  ::lampda::power::powergates::init();

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
  bool isSuccessful = true;
  std::string errorStr = "";

  // at the very last, power delivery
  const bool pdSuccess = ::lampda::power::powerDelivery::setup();
  if (!pdSuccess)
  {
    errorStr += "\n\t- Init power delivery component failed";
    isSuccessful = false;
  }

  if (not isSuccessful)
  {
    __private::powerMachine.set_state(PowerStates::ERROR);
    logic::alerts::manager.raise(logic::alerts::Type::HARDWARE_ALERT);

    set_error_state_message(errorStr);
    // failed initialisation, skip
    return;
  }

  // start power delivery
  ::lampda::power::powerDelivery::start_threads();
#endif

  // start main loop
  platform::threads::start_thread(power_loop, platform::threads::power_taskName, 0, 1024);

  isSetup = true;
}

/// main loop of the power, running in another thread.
void power_loop()
{
  // kick power watchdog
  platform::registers::kick_watchdog(POWER_WATCHDOG_ID);

  // fist action, update power gate status
  ::lampda::power::powergates::loop();

  // run power module state machine
  __private::state_machine_behavior();

  // run the power delivery update loop
  ::lampda::power::powerDelivery::loop();

  // run the charger loop (all the time)
  ::lampda::power::charger::loop();

  // run the balancer loop (all the time)
  ::lampda::power::balancer::loop();

  // after charge update, check status
  if (_isInBatteryRecoveryMode and is_started())
  {
    // remove flag when battery level is greater than zero
    const auto chargerState = ::lampda::power::charger::get_state();
    if (platform::time_ms() > startupFailTimeout_ms * 5 and chargerState.areMeasuresOk and
        chargerState.batteryVoltage_mV > batteryMinVoltageSafe_mV)
    {
      platform::lampda_print("Cleared battery recovery mode");
      _isInBatteryRecoveryMode = false;
    }
  }

  platform::delay_ms(1);
}

} // namespace power
} // namespace logic
} // namespace lampda
