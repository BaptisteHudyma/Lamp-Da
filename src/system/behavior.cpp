#include "behavior.h"

#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/system/alerts.h"

#include "src/system/power/charger.h"
#include "src/system/power/power_handler.h"
#include "src/system/power/power_gates.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/imu.h"
#include "src/system/physical/output_power.h"
#include "src/system/physical/sound.h"

#include "src/system/utils/colorspace.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/state_machine.h"
#include "src/system/utils/input_output.h"
#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/time_utils.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/print.h"

#include "src/user/functions.h"
#include "utils/print.h"

namespace behavior {

static constexpr uint32_t brightnessKey = utils::hash("brightness");
static constexpr uint32_t indicatorLevelKey = utils::hash("indLvl");

// constants
static constexpr uint32_t BRIGHTNESS_RAMP_DURATION_MS = 2000; /// duration of the brightness ramp
static constexpr uint32_t BRIGHTNESS_LOOP_UPDATE_EVERY = 20;  /// frequency update of the ramp

static constexpr uint32_t EARLY_ACTIONS_LIMIT_MS = 2000;
static constexpr uint32_t EARLY_ACTIONS_HOLD_MS = 1500;

static const uint32_t systemStartTime = time_ms();

// Define the state for the main prog state machine
typedef enum
{
  // handle the start logic
  START_LOGIC,

  // Prepare the charging operations
  PRE_CHARGER_OPERATION,
  // Charging, or usb OTG mode
  CHARGER_OPERATIONS,
  // Prepare the light operation
  PRE_OUTPUT_LIGHT,
  // output voltage on, light the led strip and enable user modes
  OUTPUT_LIGHT,
  // close the output safely
  POST_OUTPUT_LIGHT,

  // shutdown state (when set, the board wil shutdown)
  SHUTDOWN,

  // Should never happen, default state
  ERROR,
} BehaviorStates;
const char* BehaviorStatesStr[] = {
        "START_LOGIC",
        "PRE_CHARGER_OPERATION",
        "CHARGER_OPERATIONS",
        "PRE_OUTPUT_LIGHT",
        "OUTPUT_LIGHT",
        "POST_OUTPUT_LIGHT",
        "SHUTDOWN",
        "ERROR",
};
// main state machine
StateMachine<BehaviorStates> mainMachine(BehaviorStates::START_LOGIC);

// system was powered from vbus power event
static bool wokeUpFromVbus_s = false;
bool did_woke_up_from_power() { return wokeUpFromVbus_s; }

// target power on state (not actual state)
static bool isTargetPoweredOn_s = false;
bool is_system_should_be_powered() { return isTargetPoweredOn_s; }
void set_power_on() { isTargetPoweredOn_s = true; }
void set_power_off()
{
  // prevent early shutdown
  if (time_ms() - systemStartTime < 1000)
    return;

  isTargetPoweredOn_s = false;
}

// allow system to be powered if no hardware alert and power is setup
bool can_system_allowed_to_be_powered()
{
  return not(alerts::manager.is_raised(alerts::Type::SYSTEM_IN_ERROR_STATE) or not power::is_setup());
}

// return true if vbus is high
bool is_charger_powered() { return charger::is_vbus_powered(); }

// hold the last time startup_sequence has been called
uint32_t lastStartupSequence = 0;

// hold the boolean that configures if button's usermode UI is enabled
bool isButtonUsermodeEnabled = false;

// hold a boolean to avoid advertising several times in a row
bool isBluetoothAdvertising = false;

// timestamp of the system wake up
static uint32_t turnOnTime = time_ms();

bool read_parameters()
{
  // load values in memory
  const bool isFileLoaded = fileSystem::load_initial_values();

  uint32_t brightness = 0;
  if (fileSystem::get_value(brightnessKey, brightness))
  {
    brightness::update_brightness(brightness, true);
    brightness::update_previous_brightness();
  }

  uint32_t indicatorLevel = 0;
  if (fileSystem::get_value(indicatorLevelKey, indicatorLevel))
  {
    indicator::set_brightness_level(indicatorLevel);
  }

  user::read_parameters();
  return isFileLoaded;
}

void write_parameters()
{
  fileSystem::clear();

  fileSystem::set_value(brightnessKey, brightness::get_brightness());
  fileSystem::set_value(indicatorLevelKey, indicator::get_brightness_level());

  user::write_parameters();

  fileSystem::write_state();
}

// user code is running when state is output
bool is_user_code_running() { return mainMachine.get_state() == BehaviorStates::OUTPUT_LIGHT; }

/**
 * This turns off the system FOR REAL and enable the interrupt pin for power on
 * DO NOT USE THIS IF YOU ARE NOT SURE, great potential for system brick/lock
 */
void true_power_off()
{
  // stop i2c interfaces
  for (uint8_t i = 0; i < get_wire_interface_count(); ++i)
  {
    i2c_turn_off(i);
  }
  delay_ms(5);

  // disable peripherals
  DigitalPin(DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(false);
  // disable gates
#ifdef IS_HARDWARE_1_0
  // disable pullup in sleep mode (in V>1.0, done electrically)
  DigitalPin(DigitalPin::GPIO::Input_isChargeOk).set_pin_mode(DigitalPin::Mode::kInput);
  DigitalPin(DigitalPin::GPIO::Signal_BatteryBalancerAlert).set_pin_mode(DigitalPin::Mode::kInput);
#endif
  DigitalPin(DigitalPin::GPIO::Output_EnableVbusGate).set_high(false);
  DigitalPin(DigitalPin::GPIO::Output_EnableOutputGate).set_high(false);

  // deactivate indicators
  indicator::set_color(utils::ColorSpace::BLACK);
  DigitalPin::deactivate_gpios(); // physically disconnect gpios
  delay_ms(1);

  // power down nrf52.
  // on wake up, it'll start back from the setup phase
  go_to_sleep(ButtonPin.pin());

  /*
   * Nothing after this, system is off !
   */

  // if we reach this, the system failed to go to sleep, register may be broken
  alerts::manager.raise(alerts::Type::SYSTEM_OFF_FAILED);
}

void button_disable_usermode() { isButtonUsermodeEnabled = false; }

bool is_button_usermode_enabled() { return isButtonUsermodeEnabled; }

// call when the button is finally release
void button_clicked_callback(const uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  // can be called when systel is alseep
  if (consecutiveButtonCheck == 7)
  {
    indicator::set_brightness_level(indicator::get_brightness_level() + 1);
    return;
  }

  // guard blocking other actions than "turn off"
  if (not can_system_allowed_to_be_powered())
  {
    // allow turn off after a time
    if (consecutiveButtonCheck == 1)
    {
      set_power_off();
    }
    return;
  }

  // guard blocking other actions than "turning it on"
  if (not is_user_code_running())
  {
    if (consecutiveButtonCheck == 1)
    {
      set_power_on();
    }
    return;
  }

  const bool isSystemStartClick = button::is_system_start_click();

  // extended "button usermode" bypass
  if (isButtonUsermodeEnabled)
  {
    // user mode may return "True" to skip default action
    if (user::button_clicked_usermode(consecutiveButtonCheck))
    {
      return;
    }
  }

  // basic "default" UI:
  //  - 1 click: on/off
  //  - 10+ clicks: shutdown immediately (if DEBUG_MODE wait for watchdog)
  //
  switch (consecutiveButtonCheck)
  {
    // 1 click: shutdown
    case 1:
      // do not turn off on first button press
      if (not isSystemStartClick)
      {
        set_power_off();
      }
      break;

    // other behaviors
    default:
      // 10+ clicks: force shutdown (or safety reset if DEBUG_MODE)
      if (consecutiveButtonCheck >= 10)
      {
#ifdef DEBUG_MODE
        // disable charger and wait 5s to be killed by watchdog
        indicator::set_color(utils::ColorSpace::PINK);
        power::enable_charge(false);
        delay_ms(20000); // crash the system
#endif
        set_power_off();
        return;
      }

      user::button_clicked_default(consecutiveButtonCheck);
      break;
  }
}

void button_hold_callback(const uint8_t consecutiveButtonCheck, const uint32_t buttonHoldDuration)
{
  if (consecutiveButtonCheck == 0)
    return;
  // no button callback when user code is not running
  if (not is_user_code_running())
    return;

  // click chain that woke up the system
  const bool isSystemStartClick = button::is_system_start_click();

  // compute parameters of the "press-hold" action
  const bool isEndOfHoldEvent = (buttonHoldDuration <= 1);
  const uint32_t holdDuration =
          (buttonHoldDuration > HOLD_BUTTON_MIN_MS) ? (buttonHoldDuration - HOLD_BUTTON_MIN_MS) : 0;

  uint32_t realStartTime = time_ms() - lastStartupSequence;
  if (realStartTime > holdDuration)
  {
    realStartTime -= holdDuration;
  }

  //
  // "early actions"
  //    - actions to be performed by user just after lamp is turned on
  //    - after 2s the default actions comes back
  //

  if (isSystemStartClick)
  {
    // 3+hold (3s): turn it on, with button usermode enabled
    if (!isEndOfHoldEvent and consecutiveButtonCheck == 3)
    {
      if (holdDuration > EARLY_ACTIONS_HOLD_MS)
      {
        isButtonUsermodeEnabled = true;
        return;
      }
    }

    // 4+hold (3s): turn it on, with bluetooth advertising
#ifdef USE_BLUETOOTH
    if (!isEndOfHoldEvent and consecutiveButtonCheck == 4)
    {
      if (holdDuration > EARLY_ACTIONS_HOLD_MS)
      {
        if (!isBluetoothAdvertising)
        {
          bluetooth::start_advertising();
        }

        isBluetoothAdvertising = true;
        return;
      }
      return;
    }
#endif
  }

  //
  // "button usermode" bypass
  //

  if (isButtonUsermodeEnabled)
  {
    // 5+hold (5s): always exit, can't be bypassed
    if (consecutiveButtonCheck == 5)
    {
      if (holdDuration > 5000 - HOLD_BUTTON_MIN_MS)
      {
        set_power_off();
        return;
      }
    }

    // user mode may return "True" to skip default action
    if (user::button_hold_usermode(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration))
    {
      return;
    }
  }

  //
  // default actions
  //

  // number of steps to update brightness
  static constexpr uint32_t brightnessUpdateSteps = BRIGHTNESS_RAMP_DURATION_MS / BRIGHTNESS_LOOP_UPDATE_EVERY;
  static uint32_t brightnessUpdateStepSize = max(1u, maxBrightness / brightnessUpdateSteps);

  // basic "default" UI:
  //  - 1+hold: increase brightness
  //  - 2+hold: decrease brightness
  //
  switch (consecutiveButtonCheck)
  {
    // 1+hold: increase brightness
    case 1:
    // 2+hold: decrease brightness
    case 2:
      // update brightness every N milliseconds
      EVERY_N_MILLIS(BRIGHTNESS_LOOP_UPDATE_EVERY)
      {
        // update ramp every N
        if (isEndOfHoldEvent)
        {
          brightness::update_previous_brightness();
        }
        // 1 clic
        else if (consecutiveButtonCheck == 1)
        {
          const brightness_t brightness = brightness::get_brightness();
          // no updates, already at max brightness
          if (brightness >= maxBrightness)
            break;
          brightness::update_brightness(brightness + brightnessUpdateStepSize);
        }
        // 2 clics
        else
        {
          const brightness_t brightness = brightness::get_brightness();
          // no updates, already at max brightness
          if (brightness < brightnessUpdateStepSize)
            break;
          brightness::update_brightness(brightness - brightnessUpdateStepSize);
        }
      }
      break;

    case 7:
      {
        // every seconds, update the indicator
        EVERY_N_MILLIS(1000) { indicator::set_brightness_level(indicator::get_brightness_level() + 1); }
        break;
      }

    // other behaviors
    default:
      user::button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
      break;
  }
}

static std::string errorStateRaisedStr = "";
void set_error_state_message(const std::string& errorMsg)
{
  if (errorStateRaisedStr.empty())
  {
    errorStateRaisedStr = "\n\t" + errorMsg;
  }
}
std::string get_error_state_message()
{
  if (errorStateRaisedStr.empty())
    return "x";
  return errorStateRaisedStr;
}

void handle_error_state()
{
  set_error_state_message("Unspecified raised error state reason");
  outputPower::disable_power_gates();

  // not allowed to start, can only be stopped
  // turn off system is needed
  if (not is_system_should_be_powered())
  {
    // got to sleep after the closing operations
    mainMachine.set_state(BehaviorStates::SHUTDOWN);
  }

  // if error state, raise alert
  alerts::manager.raise(alerts::Type::SYSTEM_IN_ERROR_STATE);
}

void handle_start_logic_state()
{
  // prevent early charging
  power::enable_charge(false);

  // safety for failure of components at startup
  if (not can_system_allowed_to_be_powered())
  {
    set_power_on();
    set_error_state_message("system not allow to power on in start logic state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }
  if (power::is_in_error_state())
  {
    set_error_state_message("power system in error state in start logic state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }

  if (did_woke_up_from_power())
  {
    // signal to the alert manager that we started by power input
    alerts::signal_wake_up_from_charger();

    // start the charge operation
    mainMachine.set_state(BehaviorStates::PRE_CHARGER_OPERATION);
  }
  else
  {
    // small hack: the first button click is never registered, because it happens before the system boots
    set_power_on();
    // start the system normally
    mainMachine.set_state(BehaviorStates::PRE_OUTPUT_LIGHT);
  }
}

static uint32_t preChargeCalled = 0;
void handle_pre_charger_operation_state()
{
  if (power::is_in_error_state())
  {
    set_error_state_message("power system in error state in pre charger operation state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }

  preChargeCalled = time_ms();

  if (not battery::can_battery_be_charged())
  {
    // battery cannot be charged
    return;
  }

  power::go_to_charger_mode();
  mainMachine.set_state(BehaviorStates::CHARGER_OPERATIONS);
}

void handle_charger_operation_state()
{
  if (power::is_in_error_state())
  {
    set_error_state_message("power system in error state in charger operation state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }

  // pressed the start button, stop charge and start lamp
  if (is_system_should_be_powered())
  {
    // forbid charging
    power::enable_charge(false);
    yield_this_thread();

    // switch to output mode after the post charge operations
    mainMachine.set_state(BehaviorStates::PRE_OUTPUT_LIGHT);
    return;
  }
  // wait a bit after going to charger mode, maybe vbus is bouncing around
  const bool vbusDebounced = charger::can_use_vbus_power() or (time_ms() - preChargeCalled) > 5000;
  if (vbusDebounced)
  {
    // otg mode
    if (charger::get_state().isInOtg)
    {
      // do nothing (for now !)
      // TODO issue #133, stop if battery gets low, or temperature high
    }
    // no power, shutdown everything
    else if (not is_charger_powered())
    {
      // forbbid charging
      power::enable_charge(false);
      yield_this_thread();

      // go to sleep after closing the charger
      mainMachine.set_state(BehaviorStates::SHUTDOWN);
      return;
    }
    else
    {
      // enable charge after the debounce
      power::enable_charge(true);
    }
  }
  // else: ignore all
}

static uint32_t lastOutputLightValidTime = 0;

void handle_pre_output_light_state()
{
  if (power::is_in_error_state())
  {
    set_error_state_message("power system in error state in pre output light state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }

  // critical battery level, do not wake up
  if (battery::get_battery_minimum_cell_level() <= batteryCritical + 1 or
      not battery::is_battery_usable_as_power_source())
  {
    // alert user of low battery
    for (uint8_t i = 0; i < 10; i++)
    {
      indicator::set_color(utils::ColorSpace::RED);
      delay_ms(100);
      indicator::set_color(utils::ColorSpace::BLACK);
      delay_ms(100);
    }

    if (is_charger_powered())
    {
      // go to battery charge instead of shutdown
      mainMachine.set_state(BehaviorStates::PRE_CHARGER_OPERATION);
    }
    else
    {
      // battery too low to be powered
      mainMachine.set_state(BehaviorStates::SHUTDOWN);
    }
    return;
  }

  power::go_to_output_mode();

  // button usermode is always disabled by default
  button_disable_usermode();

  // reset lastStartupSequence
  lastStartupSequence = time_ms();

  // let the user power on the system
  user::power_on_sequence();

  lastOutputLightValidTime = time_ms();

  // this function is executed OUNCE
  mainMachine.set_state(BehaviorStates::OUTPUT_LIGHT);
}

void handle_output_light_state()
{
  if (power::is_in_error_state())
  {
    set_error_state_message("power system in error state in output light state");
    mainMachine.set_state(BehaviorStates::ERROR);
    return;
  }

// TODO issue #132 remove when the mock threads will be running
#ifndef LMBD_SIMULATION
  static bool waitingForPowerGate_messageDisplayed = true;

  // wait for power gates (and display message when ready)
  if (not powergates::is_power_gate_enabled() or not power::is_output_mode_ready())
  {
    if (waitingForPowerGate_messageDisplayed)
    {
      lampda_print("Behavior>Output mode: waiting for power gate");
      waitingForPowerGate_messageDisplayed = false;
    }

    if (time_ms() - lastOutputLightValidTime > 1000)
    {
      set_error_state_message("power gate took too long to switch in output light state " +
                              std::to_string(powergates::is_power_gate_enabled()) +
                              std::to_string(power::is_output_mode_ready()));
      mainMachine.set_state(BehaviorStates::ERROR);
    }
    return;
  }
  else if (not waitingForPowerGate_messageDisplayed)
  {
    lampda_print("Behavior>Output mode: power gate ready");
    waitingForPowerGate_messageDisplayed = true;
  }
#endif

  lastOutputLightValidTime = time_ms();

  // should go to sleep
  if (not is_system_should_be_powered())
  {
    if (is_charger_powered())
    {
      // charger is plugged, go to charger state
      mainMachine.set_state(BehaviorStates::POST_OUTPUT_LIGHT, 100, BehaviorStates::PRE_CHARGER_OPERATION);
    }
    else
    {
      // got to sleep after the closing operations
      mainMachine.set_state(BehaviorStates::POST_OUTPUT_LIGHT, 1000, BehaviorStates::SHUTDOWN);
    }
  }
  // normal running loop
  else
  {
    // user loop call
    user::loop();

// TODO issue #136
#if 0
    const auto& chargerState = charger::get_state();
    if (chargerState.status == charger::Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING)
    {
      // TODO: alert that the battery is missing
    }
#endif
  }
}

void handle_post_output_light_state()
{
  // button usermode is kept disabled
  button_disable_usermode();

  // let the user power off the system
  user::power_off_sequence();
  // write/stability delay
  delay_ms(10);

  // deactivate strip power
  outputPower::disable_power_gates(); // close external voltage path
  delay_ms(1);
  outputPower::write_voltage(0); // power down

  mainMachine.skip_timeout();
}

void handle_shutdown_state()
{
  // detach all interrupts, to prevent interruption of shutdown
  DigitalPin::detach_all();
  yield_this_thread();

  // shutdown all external power
  if (not power::go_to_shutdown())
  {
    // TODO: error ?
  }

  uint8_t maxChecks = 100;
  while (is_all_suspended() != 1 and maxChecks > 0)
  {
    maxChecks--;
    // block other threads
    suspend_all_threads();
    yield_this_thread();
    delay_ms(5);
  }
  if (maxChecks == 0)
  {
    // some task have refused to power off !!
    // TODO : alert ?
  }

  // deactivate strip power
  outputPower::write_voltage(0); // power down
  delay_ms(10);

  // disable bluetooth, imu and microphone
  microphone::disable();
  imu::shutdown();
#ifdef USE_BLUETOOTH
  bluetooth::stop_bluetooth_advertising();
#endif

  // save the current config to a file
  // (takes some time so call it when the lamp appear to be shutdown already)
  write_parameters();

  // power the system off
  true_power_off();
}

/// Handle the behavior states
void state_machine_behavior()
{
  mainMachine.run();
  // if state changed, display the new state
  if (mainMachine.state_just_changed())
  {
    lampda_print("BEHAVIOR_S_MACH > switched to state %s", BehaviorStatesStr[mainMachine.get_state()]);
  }

  switch (mainMachine.get_state())
  {
    // strange error state
    case BehaviorStates::ERROR:
      handle_error_state();
      break;
    // called when system starts
    case BehaviorStates::START_LOGIC:
      handle_start_logic_state();
      break;
    // prepare the charger operation
    case BehaviorStates::PRE_CHARGER_OPERATION:
      handle_pre_charger_operation_state();
      break;
    // charge batteries, or power OTG on USB-port
    case BehaviorStates::CHARGER_OPERATIONS:
      handle_charger_operation_state();
      break;
    // prepare the output light
    case BehaviorStates::PRE_OUTPUT_LIGHT:
      handle_pre_output_light_state();
      break;
    // output current to system, handle user inputs
    case BehaviorStates::OUTPUT_LIGHT:
      handle_output_light_state();
      break;
    case BehaviorStates::POST_OUTPUT_LIGHT:
      handle_post_output_light_state();
      break;
    case BehaviorStates::SHUTDOWN:
      handle_shutdown_state();
      break;
    default:
      {
        set_error_state_message("reached state machine default state");
        mainMachine.set_state(BehaviorStates::ERROR);
      }
      break;
  }
}

// system was powered by vbus high event
void set_woke_up_from_vbus(const bool wokeUp) { wokeUpFromVbus_s = wokeUp; }

// main behavior loop
void loop()
{
  state_machine_behavior();

  // do not display alerts for the first 500 ms
  const bool shouldIgnoreAlerts = (time_ms() - turnOnTime) < 500;
  alerts::handle_all(shouldIgnoreAlerts);
  // alert requested an emergency shutdown, do it
  if (alerts::is_request_shutdown())
  {
    lampda_print("emergency shutdown from alert");
    // just in case, turn off eventual states
    handle_post_output_light_state();
    // shutdown normally
    handle_shutdown_state();
  }
}

std::string get_state() { return std::string(BehaviorStatesStr[mainMachine.get_state()]); }

} // namespace behavior

//
// do not modify (if you do not understand it)
//

#ifdef LMBD_CPP17

const char* ensure_build_canary()
{
#warning "compiling ensure_build_canary"

#ifdef LMBD_LAMP_TYPE__SIMPLE
  return "_lmbd__build_canary__simple";
#endif
#ifdef LMBD_LAMP_TYPE__CCT
  return "_lmbd__build_canary__cct";
#endif
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  return "_lmbd__build_canary__indexable";
#endif
  return (char*)nullptr;
}

#endif
