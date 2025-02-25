#include "power_handler.h"

#include "src/system/utils/print.h"
#include "src/system/utils/state_machine.h"

namespace power {

static constexpr uint32_t clearPowerRailFailureDelay_ms = 1000;

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

} // namespace __private

// TODO
uint32_t get_power_rail_voltage() { return 5000; }

void handle_clear_power_rails()
{
  // TODO: ensure all gates are closed
  // lock output gate
  // lock vbus gate

  // min value for rail voltage is 3200mV
  // if (get_power_rail_voltage() <= 3200) {
  // all is clear, skip to next step
  __private::powerMachine.skip_timeout();
  /*}
  else // discharge power rail
  {
    // TODO: activate/control discharge

    // timeout after which we go to error ?
    if (time_ms() - __private::powerMachine.get_state_raised_time() >= clearPowerRailFailureDelay_ms)
    {
      __private::powerMachine.set_state(PowerStates::ERROR);
    }
  }*/
}

void handle_charging_mode()
{
  // charge OR idle and do nothing (end of charge)
}

void handle_output_voltage_mode()
{
  // ramp up output voltage
  // then unlock the output gate

  // allow control of output voltage
}

void handle_otg_mode()
{
  // ramp up output voltage
  // then unlock the vbus gate
}

void handle_shutdown()
{
  // lock vbus gate
  // lock output gate

  // set high impedence for power ic
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
  __private::switch_state(PowerStates::SHUTDOWN);
  return true;
}

bool set_output_voltage_mv(const uint32_t) { return false; }

bool set_output_max_current_mA(const uint32_t) { return false; }

// charge mode
bool enable_charge(const bool) { return false; }

// otg mode
bool set_otg_voltage_mv(const uint32_t) { return false; }
bool set_otg_max_current_mA(const uint32_t) { return false; }

void init()
{
  // TODO: enable and set the power component parameters

  __private::switch_state(PowerStates::IDLE);
}

void loop()
{
  // run power module state machine
  __private::state_machine_behavior();

  // run the pd negociation
}

} // namespace power