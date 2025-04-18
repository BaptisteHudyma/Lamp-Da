#include "power_handler.h"

#include "src/system/utils/print.h"
#include "src/system/utils/state_machine.h"

#include "src/system/physical/battery.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/threads.h"

#include "PDlib/power_delivery.h"
#include "balancer.h"
#include "charger.h"
#include "power_gates.h"
#include <cstdint>

namespace power {

bool _isShutdownCompleted = false;

static constexpr uint32_t clearPowerRailMinDelay_ms = 10;
static constexpr uint32_t clearPowerRailFailureDelay_ms = 1000;

static_assert(clearPowerRailMinDelay_ms < clearPowerRailFailureDelay_ms,
              "clear power rail min activation is less than min unlock delay");

static uint16_t _outputVoltage_mV = 0;
static uint16_t _outputCurrent_mA = 0;
static bool _isChargeEnabled = false;
static std::string _errorStr = "";

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
      _errorStr = "CLEAR_POWER_RAILS: discharge VBUS failed";
      __private::powerMachine.set_state(PowerStates::ERROR);
    }
  }
}

void handle_charging_mode()
{
  // allow OTG in charge mode only
  powerDelivery::allow_otg(true);
  // open vbus
  powergates::enable_vbus_gate();

  // OTG requested, switch to OTG mode
  if (powerDelivery::is_switching_to_otg() and battery::is_battery_usable_as_power_source())
  {
    // disable balancing
    balancer::enable_balancing(false);
    // start otg
    go_to_otg_mode();
    return;
  }

  if (powergates::is_vbus_gate_enabled())
  {
    charger::set_enable_charge(_isChargeEnabled);
    // balance while we have power on vbus
    balancer::enable_balancing(true);
  }

  // charge OR idle and do nothing (end of charge)
}

void handle_output_voltage_mode()
{
  set_otg_parameters(_outputVoltage_mV, _outputCurrent_mA);

  // open power gate when voltage matches expected voltage
  uint32_t vbusVoltage = get_power_rail_voltage();
  if (vbusVoltage >= _outputVoltage_mV * 0.9 and vbusVoltage <= _outputVoltage_mV * 1.1)
  {
    // close power gate
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
  const auto requested = powerDelivery::get_otg_parameters();
  // we do not have the parameters yet
  if (not requested.is_otg_requested())
  {
    return;
  }

  charger::set_enable_charge(false);

  // ramp up output voltage
  // then unlock the vbus gate
  set_otg_parameters(requested.requestedVoltage_mV, requested.requestedCurrent_mA);

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
        _errorStr = "default case not handled";
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

// control parameters
bool go_to_output_mode()
{
  if (__private::can_switch_states())
  {
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
    __private::switch_state(PowerStates::CHARGING_MODE);
    return true;
  }

  return false;
}

bool go_to_otg_mode()
{
  if (__private::can_switch_states())
  {
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

bool set_output_voltage_mv(const uint16_t outputVoltage_mV)
{
  _outputVoltage_mV = outputVoltage_mV;
  return false;
}

bool set_output_max_current_mA(const uint16_t outputCurrent_mA)
{
  _outputCurrent_mA = outputCurrent_mA;
  return false;
}

// charge mode
bool enable_charge(const bool enable)
{
  _isChargeEnabled = enable;
  return true;
}

std::string get_state() { return std::string(PowerStatesStr[__private::powerMachine.get_state()]); }
std::string get_error_string() { return _errorStr; }

bool is_in_output_mode() { return __private::powerMachine.get_state() == PowerStates::OUTPUT_VOLTAGE_MODE; }
bool is_in_otg_mode() { return __private::powerMachine.get_state() == PowerStates::OTG_MODE; }

static bool isSetup = false;
void init()
{
  powergates::init();

  // switch without a timing
  __private::powerMachine.set_state(PowerStates::IDLE);

  if (not balancer::init())
  {
    // TODO:
    // nothing ? the lamp can work without this
  }

  // charging component, setup first
  const bool chargerSuccess = charger::setup();
  if (!chargerSuccess)
  {
    __private::switch_state(PowerStates::ERROR);
    return;
  }

  // at the very last, power delivery
  const bool pdSuccess = powerDelivery::setup();
  if (!pdSuccess)
  {
    __private::switch_state(PowerStates::ERROR);
    return;
  }
  isSetup = true;
}

void start_threads()
{
  if (!isSetup)
    return;

  //   use the charging thread !
  start_thread(loop, power_taskName, 0, 1024);

  powerDelivery::start_threads();
}

void loop()
{
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

} // namespace power