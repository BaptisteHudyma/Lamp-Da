#include "power_handler.h"

#include "src/system/utils/print.h"
#include "src/system/utils/state_machine.h"

#include "src/system/platform/gpio.h"

#include "PDlib/power_delivery.h"
#include "charger.h"

namespace power {

static constexpr uint32_t clearPowerRailFailureDelay_ms = 1000;

static uint16_t _otgVoltage_mV = 0;
static uint16_t _otgCurrent_mA = 0;
static uint16_t _outputVoltage_mV = 0;
static uint16_t _outputCurrent_mA = 0;

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

namespace __private {

// main state machine (start in error to force user to init)
StateMachine<PowerStates> powerMachine(PowerStates::ERROR);

DigitalPin enableVbusGate(DigitalPin::GPIO::Output_EnableVbusGate);
DigitalPin enablePowerGate(DigitalPin::GPIO::Output_EnableOutputGate);
DigitalPin dischargeVbus(DigitalPin::GPIO::Output_DischargeVbus);

} // namespace __private

void disable_gates()
{
  __private::dischargeVbus.set_high(false);
  __private::enableVbusGate.set_high(false);
  __private::enablePowerGate.set_high(false);
}

void enable_vbus_gate()
{
  __private::dischargeVbus.set_high(false);

  if (not __private::enablePowerGate.is_high())
  {
    __private::enableVbusGate.set_high(true);
  }
  else
  {
    // error !
  }
}

void enable_power_gate()
{
  __private::dischargeVbus.set_high(false);

  if (not __private::enableVbusGate.is_high())
  {
    __private::enablePowerGate.set_high(true);
  }
  else
  {
    // error !
  }
}

uint32_t get_power_rail_voltage()
{
  const auto& state = charger::get_state();
  if (state.areMeasuresOk)
  {
    return state.vbus_mV;
  }
  else
  {
    // temp value, will lock the gates until measures are good
    return 5000;
  }
}

void handle_clear_power_rails()
{
  // close all gates
  disable_gates();

  // min value for rail voltage is 3200mV
  if (get_power_rail_voltage() <= 3200)
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
      __private::powerMachine.set_state(PowerStates::ERROR);
    }
  }
}

void handle_charging_mode()
{
  // open vbus
  enable_vbus_gate();

  // charge OR idle and do nothing (end of charge)
}

void handle_output_voltage_mode()
{
  // open vbus
  // enable_power_gate();

  static uint16_t lastOtgVoltage_mV = 0;
  static uint16_t lastOtgCurrent_mA = 0;
  if (lastOtgVoltage_mV != _outputVoltage_mV || lastOtgCurrent_mA != _outputCurrent_mA)
  {
    lastOtgVoltage_mV = _outputVoltage_mV;
    lastOtgCurrent_mA = _outputCurrent_mA;

    // ramp up output voltage
    charger::control_OTG(_outputVoltage_mV, _outputCurrent_mA);
  }

  // allow control of output voltage
}

void handle_otg_mode()
{
  // ramp up output voltage
  // then unlock the vbus gate
  // charger::control_OTG(_otgVoltage_mV, _otgCurrent_mA);

  // open vbus
  enable_vbus_gate();
}

void handle_shutdown()
{
  // close all gates
  disable_gates();

  powerDelivery::shutdown();

  // shutdown charger component
  charger::shutdown();
}

void handle_error_state()
{
  //
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

bool go_to_shutdown()
{
  // no need to check if we can switch case in this case
  __private::powerMachine.set_state(PowerStates::SHUTDOWN);
  return true;
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
  charger::set_enable_charge(enable);
  return true;
}

// otg mode
bool set_otg_voltage_mv(const uint32_t otgVolage_mV)
{
  _otgVoltage_mV = otgVolage_mV;
  return false;
}

bool set_otg_max_current_mA(const uint32_t otgVCurrent_mA)
{
  _otgCurrent_mA = otgVCurrent_mA;
  return false;
}

std::string get_state() { return std::string(PowerStatesStr[__private::powerMachine.get_state()]); }

bool is_in_output_mode() { return __private::powerMachine.get_state() == PowerStates::OUTPUT_VOLTAGE_MODE; }

void init()
{
  disable_gates();

  const bool success = powerDelivery::setup();
  if (!success)
  {
    __private::switch_state(PowerStates::ERROR);
    return;
  }
  // switch without a timing
  __private::powerMachine.set_state(PowerStates::IDLE);
  disable_gates();

  charger::setup();
}

void loop()
{
  // run power module state machine
  __private::state_machine_behavior();

  // run pd negociation loop
  powerDelivery::loop();

  // run the charger loop (all the time)
  charger::loop();
}

} // namespace power