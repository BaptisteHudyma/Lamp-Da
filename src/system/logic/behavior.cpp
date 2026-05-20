#include "behavior.h"

#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/inputs_bluetooth.h"
#include "src/system/logic/inputs.h"
#include "src/system/logic/statistics_handler.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/power/charger.h"
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
#include "src/system/utils/time_utils.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/print.h"

#include "src/user/functions.h"

namespace lampda {
namespace logic {
namespace behavior {

static constexpr uint32_t cleanSleepKey = utils::hash("cleanSleep");
static constexpr uint32_t brightnessKey = utils::hash("brightness");
static constexpr uint32_t indicatorLevelKey = utils::hash("indLvl");
static constexpr uint32_t isLockoutModeKey = utils::hash("lckMode");
static constexpr uint32_t buttonPinKey = utils::hash("bttPin");
static constexpr uint32_t bluetoothAutoKey = utils::hash("ble");

// time to block turn off since turn on
static constexpr uint32_t SYSTEM_TURN_ON_ALLOW_TURN_OFF_DELAY = 500;
static constexpr uint32_t PRE_CHARGE_STATE_TIMEOUT_ms = 1000;
static constexpr uint32_t PRE_OUTPUT_STATE_TIMEOUT_ms = 1000;

/// bluetooth will power up automatically this number of next boots if user used it
static constexpr uint32_t maxBluetoothAutoActivations = 3;

// timestamp of the system wake up
static uint32_t wakeUpTime = 0;

// pre output light call timing (lamp output starts)
// Starts at system start time
static uint32_t preOutputLightCalled = 0;

// indicates that the system was waken up by voltage on vbus
static bool wokeUpFromVbus_s = false;
// indicates that the output should be turned on/off
static bool isTargetPoweredOn_s = false;
// indicates the error that started the error state
static std::string errorStateRaisedStr = "";
// indicates the time at which the pre-charge state started
static uint32_t preChargeCalled = 0;
// indicates the  time at which the output state is enabled
static uint32_t lastOutputLightValidTime = 0;
/// keep track of the number of auto bluetooth activation left
static uint32_t bluetoothAutoActivationLeftCount = 0;

// Define the state for the main prog state machine
using BehaviorStates = enum class behavior_t
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
};
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
utils::StateMachine<BehaviorStates> mainMachine(BehaviorStates::START_LOGIC);

// system was powered from vbus power event
bool did_woke_up_from_power() { return wokeUpFromVbus_s; }

// target power on state (not actual state)
bool is_system_should_be_powered() { return isTargetPoweredOn_s; }
void set_power_on() { isTargetPoweredOn_s = true; }
void set_power_off()
{
  // prevent early shutdown when system is just booting, or lamp jst turns on
  if (platform::time_ms() - preOutputLightCalled < SYSTEM_TURN_ON_ALLOW_TURN_OFF_DELAY)
    return;

  isTargetPoweredOn_s = false;
}

void go_to_external_battery_mode() { logic::power::go_to_otg_mode(); }

//
bool is_in_start_logic_state() { return mainMachine.get_state() == BehaviorStates::START_LOGIC; }
bool is_in_output_state() { return mainMachine.get_state() == BehaviorStates::OUTPUT_LIGHT; }
bool is_in_charge_state() { return mainMachine.get_state() == BehaviorStates::CHARGER_OPERATIONS; }

namespace sunset {

// signal update to user
void progress_update(const float progress) { user::sunset_timer_update(progress); }

} // namespace sunset

// allow system to be powered if no hardware alert and power is setup
bool can_system_allowed_to_be_powered()
{
  return not(logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_ERROR_STATE) or
             not logic::power::is_setup());
}

// return true if vbus is high
bool is_charger_powered() { return ::lampda::power::charger::is_vbus_powered(); }

bool read_parameters()
{
  bool isSuccess = false;
  // load system values in memory
  if (not physical::fileSystem::system::load_from_file())
  {
    logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_SLEEP_SKIPPED);
  }
  else
  {
    isSuccess = true;
    // load statistics first
    statistics::load_from_memory();

    uint32_t wasPutToSleepCleanly = 0;
    const bool isCleanFlagFound = physical::fileSystem::system::get_value(cleanSleepKey, wasPutToSleepCleanly);
    if (not isCleanFlagFound or wasPutToSleepCleanly != 0xDEADBEEF)
    {
      // dirty sleep alert
      logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_SLEEP_SKIPPED);
    }

    uint32_t brightness = 0;
    if (physical::fileSystem::system::get_value(brightnessKey, brightness))
    {
      logic::brightness::update_brightness(brightness, true);
      logic::brightness::update_saved_brightness();
    }

    uint32_t indicatorLevel = 0;
    if (physical::fileSystem::system::get_value(indicatorLevelKey, indicatorLevel))
    {
      logic::indicator::set_brightness_level(indicatorLevel);
    }

    uint32_t isInLockoutMode = 0;
    if (physical::fileSystem::system::get_value(isLockoutModeKey, isInLockoutMode) and isInLockoutMode != 0)
    {
      // system in lockout, raise the alert
      logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
    }

    uint32_t buttonPin = 0;
    if (physical::fileSystem::system::get_value(buttonPinKey, buttonPin) and buttonPin > 0)
    {
      physical::button::set_button_pin(static_cast<platform::gpio::DigitalPin::GPIO>(buttonPin));
    }

    // Auto activate bluetooth is needed
    uint32_t bluetoothAutoActivation = 0;
    if (physical::fileSystem::system::get_value(bluetoothAutoKey, bluetoothAutoActivation) and
        bluetoothAutoActivation > 0)
    {
      bluetoothAutoActivationLeftCount = min<uint32_t>(maxBluetoothAutoActivations, bluetoothAutoActivation - 1);

      platform::bluetooth::start_advertising();
    }
    else
    {
      bluetoothAutoActivationLeftCount = 0;
    }
  }

  if (physical::fileSystem::user::load_from_file())
  {
    user::read_parameters();
  }
  // else: we can live without the user parameters

  return isSuccess;
}

void setup_clean_sleep_flag()
{
  // clear clean sleep flag
  physical::fileSystem::system::set_value(cleanSleepKey, 0);
  // write parameters, if a crash happens, we will notice a dirty flag
  physical::fileSystem::system::write_to_file();
  // temp shutdown
  physical::fileSystem::shutdown();
}

void write_parameters(const bool shouldSaveUserParameters = true, const bool shouldSaveSystemParameters = true)
{
  physical::fileSystem::clear();

  if (shouldSaveSystemParameters)
  {
    // write updated statistics
    statistics::write_to_memory();

    physical::fileSystem::system::set_value(cleanSleepKey, 0xDEADBEEF);
    // only save saved brightness, not current
    physical::fileSystem::system::set_value(brightnessKey, logic::brightness::get_saved_brightness());
    physical::fileSystem::system::set_value(indicatorLevelKey, logic::indicator::get_brightness_level());
    // lockout mode always kept, if not deactivated by system
    physical::fileSystem::system::set_value(isLockoutModeKey,
                                            logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT));
    physical::fileSystem::system::set_value(buttonPinKey, static_cast<uint32_t>(physical::button::get_button_pin()));

    // if the user used the bluetooth, it will be written here for auto activation
    const uint32_t nextWakeUpWithBluetooth = logic::inputs_bluetooth::is_bluetooth_used() ?
                                                     maxBluetoothAutoActivations :
                                                     bluetoothAutoActivationLeftCount;
    physical::fileSystem::system::set_value(bluetoothAutoKey, nextWakeUpWithBluetooth);
  }
  else
  {
    physical::fileSystem::clear_system_parameters();
  }

  if (shouldSaveUserParameters)
    user::write_parameters();

  // write all
  physical::fileSystem::user::write_to_file();
  physical::fileSystem::system::write_to_file();
  // close filesystem
  physical::fileSystem::shutdown();
}

// user code is running when state is output
bool is_user_code_running() { return mainMachine.get_state() == BehaviorStates::OUTPUT_LIGHT; }

/**
 * This turns off the system FOR REAL and enable the interrupt pin for power on
 * DO NOT USE THIS IF YOU ARE NOT SURE, great potential for system brick/lock
 */
void true_power_off()
{
  // unmount filesystem
  physical::fileSystem::shutdown();

  // stop i2c interfaces
  for (uint8_t i = 0; i < platform::registers::get_wire_interface_count(); ++i)
  {
    platform::i2c::i2c_turn_off(i);
  }
  platform::delay_ms(5);

  // disable peripherals
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(false);
  // disable gates
#ifdef IS_HARDWARE_1_0
  // disable pullup in sleep mode (in V>1.0, done electrically)
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Input_isChargeOk)
          .set_pin_mode(platform::gpio::DigitalPin::Mode::kInput);
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Signal_BatteryBalancerAlert)
          .set_pin_mode(platform::gpio::DigitalPin::Mode::kInput);
#endif
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Output_EnableVbusGate).set_high(false);
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Output_EnableOutputGate).set_high(false);

  // deactivate indicators
  physical::indicator::set_color(utils::ColorSpace::BLACK);
  platform::gpio::DigitalPin::deactivate_gpios(); // physically disconnect gpios
  platform::delay_ms(1);

  // power down nrf52.
  // on wake up, it'll start back from the setup phase
  platform::registers::go_to_sleep(physical::button::get_button_pin_RAW());

  /*
   * Nothing after this, system is off !
   */

  // if we reach this, the system failed to go to sleep, register may be broken
  logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_OFF_FAILED);
}

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

void go_to_error_state(const std::string& errorMsg)
{
  statistics::signal_output_off();

  // signal to the system we dont want to turn off until user asked
  // thats better to debug an error
  set_power_on();
  // set error msg
  set_error_state_message(errorMsg);
  // got to error state
  mainMachine.set_state(BehaviorStates::ERROR);
}

namespace internal {

void handle_error_state()
{
  set_error_state_message("Unspecified raised error state reason");

  // not allowed to start, can only be stopped
  // turn off system is needed
  if (not is_system_should_be_powered())
  {
    // got to sleep after the closing operations
    mainMachine.set_state(BehaviorStates::SHUTDOWN);
  }

  // power states go to error too
  logic::power::go_to_error();

  // kick power and user watchdog (prevent reset)
  platform::registers::kick_watchdog(POWER_WATCHDOG_ID);
  platform::registers::kick_watchdog(USER_WATCHDOG_ID);

  // if error state, raise alert
  logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_IN_ERROR_STATE);
}

void handle_start_logic_state()
{
  // set clean flag early on
  setup_clean_sleep_flag();

  // prevent early charging
  logic::power::enable_charge(false);

  // safety for failure of components at startup
  if (not can_system_allowed_to_be_powered())
  {
    go_to_error_state("system not allow to power on in start logic state");
    return;
  }
  if (logic::power::is_in_error_state())
  {
    go_to_error_state("power system in error state in start logic state");
    return;
  }
  if (not logic::power::is_started())
  {
    // wait until power is started
    platform::delay_ms(1);
    return;
  }

  //
  if (did_woke_up_from_power())
  {
    // signal to the alert manager that we started by power input
    logic::alerts::signal_wake_up_from_charger();

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

void handle_pre_charger_operation_state()
{
  if (logic::power::is_in_error_state())
  {
    go_to_error_state("power system in error state in pre charger operation state");
    return;
  }
  if (platform::time_ms() - mainMachine.get_state_raised_time() > PRE_CHARGE_STATE_TIMEOUT_ms)
  {
    go_to_error_state("pre charger operation failed to toggle to charge");
    return;
  }

  // clear lockout in charge mode
  logic::alerts::manager.clear(logic::alerts::Type::SYSTEM_IN_LOCKOUT);

  preChargeCalled = platform::time_ms();

  if (not physical::battery::can_battery_be_charged())
  {
    // battery cannot be charged
    return;
  }
  if (not logic::alerts::manager.can_use_usb_port())
  {
    // USB port use is locked
    return;
  }

  // force OTG
  if (not logic::power::is_in_otg_mode())
    logic::power::go_to_charger_mode();
  mainMachine.set_state(BehaviorStates::CHARGER_OPERATIONS);
}

void handle_charger_operation_state()
{
  if (logic::power::is_in_error_state())
  {
    go_to_error_state("power system in error state in charger operation state");
    return;
  }

  // pressed the start button, stop charge and start lamp
  if (is_system_should_be_powered())
  {
    // forbid charging
    logic::power::enable_charge(false);
    platform::threads::yield_this_thread();

    // switch to output mode after the post charge operations
    mainMachine.set_state(BehaviorStates::PRE_OUTPUT_LIGHT);
    return;
  }
  // wait a bit after going to charger mode, maybe vbus is bouncing around
  const bool vbusDebounced =
          ::lampda::power::charger::can_use_vbus_power() or (platform::time_ms() - preChargeCalled) > 5000;
  if (vbusDebounced)
  {
    static uint32_t otgDebounce_time = 0;
    if (::lampda::power::charger::get_state().isInOtg)
    {
      otgDebounce_time = platform::time_ms();
    }

    // otg mode
    if ((otgDebounce_time != 0) and (platform::time_ms() - otgDebounce_time) <= 500)
    {
      // do nothing (for now !)
    }
    // no power, shutdown everything
    else if (not is_charger_powered())
    {
      // forbbid charging
      logic::power::enable_charge(false);
      platform::threads::yield_this_thread();

      // go to sleep after closing the charger
      mainMachine.set_state(BehaviorStates::SHUTDOWN);
      return;
    }
    else
    {
      // enable charge after the debounce
      logic::power::enable_charge(true);
    }
  }
  // else: ignore all
}

bool check_handle_exit_output_mode()
{
  // should go to sleep
  if (not is_system_should_be_powered())
  {
    logic::sunset::cancel_timer();

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
    return true;
  }
#ifndef LMBD_SIMULATION
  else if (not physical::battery::is_battery_usable_as_power_source())
  {
    logic::sunset::cancel_timer();

    // wait a bit then shutdown
    if (platform::time_ms() - preOutputLightCalled > 3000)
    {
      // indicate we should power the systemm off
      set_power_off();
    }
    return true;
  }
#endif

  return false;
}

void handle_pre_output_light_state()
{
  static bool isMessageDisplayed = false;

  if (logic::power::is_in_error_state())
  {
    go_to_error_state("power system in error state in pre output light state");
    return;
  }
  if (platform::time_ms() - mainMachine.get_state_raised_time() > PRE_OUTPUT_STATE_TIMEOUT_ms)
  {
    go_to_error_state("pre output operation failed to toggle");
    return;
  }

  // power usage is forbidden
  if (not physical::battery::is_battery_usable_as_power_source())
  {
    if (not isMessageDisplayed)
    {
      platform::lampda_print("no output allowed");
      isMessageDisplayed = true;
    }
    // check if we may go to sleep
    check_handle_exit_output_mode();
    return;
  }

  if (logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT))
  {
    if (not isMessageDisplayed)
    {
      platform::lampda_print("not output in lockout state");
      isMessageDisplayed = true;
    }
    // check if we may go to sleep
    check_handle_exit_output_mode();
    return;
  }

  //
  isMessageDisplayed = false;

  // critical battery level, do not wake up
  if (physical::battery::get_battery_minimum_cell_level() <= batteryCritical + 1)
  {
    // alert user of low battery
    for (uint8_t i = 0; i < 10; i++)
    {
      physical::indicator::set_color(utils::ColorSpace::RED);
      platform::delay_ms(100);
      physical::indicator::set_color(utils::ColorSpace::BLACK);
      platform::delay_ms(100);
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
  preOutputLightCalled = platform::time_ms();

  logic::power::go_to_output_mode();

  // update brightness with saved brightness
  logic::brightness::update_brightness(logic::brightness::get_saved_brightness());

  // let the user power on the system
  user::power_on_sequence();

  lastOutputLightValidTime = platform::time_ms();

  // this function is executed ONCE
  statistics::signal_output_on();
  mainMachine.set_state(BehaviorStates::OUTPUT_LIGHT);
}

void handle_output_light_state()
{
  if (logic::power::is_in_error_state())
  {
    go_to_error_state("power system in error state in output light state");
    return;
  }

  // shortcut to external battery mode
  if (logic::power::is_in_otg_mode())
  {
    // ignore timing, special case
    isTargetPoweredOn_s = false;
    mainMachine.set_state(BehaviorStates::POST_OUTPUT_LIGHT, 100, BehaviorStates::PRE_CHARGER_OPERATION);
    return;
  }

  static bool waitingForPowerGate_messageDisplayed = true;

  // wait for power gates (and display message when ready)
  if (not ::lampda::power::powergates::is_power_gate_enabled() or not logic::power::is_output_mode_ready())
  {
    if (waitingForPowerGate_messageDisplayed)
    {
      platform::lampda_print("Behavior>Output mode: waiting for power gate");
      waitingForPowerGate_messageDisplayed = false;
    }

    if (platform::time_ms() - lastOutputLightValidTime > 1000)
    {
      go_to_error_state("power gate took too long to switch in output light state " +
                        std::to_string(::lampda::power::powergates::is_power_gate_enabled()) +
                        std::to_string(logic::power::is_output_mode_ready()));
    }
    return;
  }
  waitingForPowerGate_messageDisplayed = true;

  lastOutputLightValidTime = platform::time_ms();

  // normal running loop
  if (not check_handle_exit_output_mode())
  {
    // resume secondary thread
    if (user::should_spawn_thread())
      platform::threads::resume_thread(platform::threads::user_taskName);

    // user loop call
    user::loop();

    const auto& chargerState = ::lampda::power::charger::get_state();
    if (chargerState.status == ::lampda::power::charger::Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING)
    {
      logic::alerts::manager.raise(logic::alerts::Type::BATTERY_MISSING);
    }
  }
}

void handle_post_output_light_state()
{
  // button usermode is kept disabled
  inputs::button_disable_usermode();

  // let the user power off the system
  user::power_off_sequence();
  // write/stability delay
  platform::delay_ms(10);

  // deactivate strip power
  physical::outputPower::disable_power_gates(); // close external voltage path
  platform::delay_ms(1);
  physical::outputPower::write_voltage(0); // power down

  statistics::signal_output_off();
  mainMachine.skip_timeout();
}

void handle_shutdown_state(const bool shouldSaveUserParameters, const bool shouldSaveSystemParameters)
{
  // detach all interrupts, to prevent interruption of shutdown
  platform::gpio::DigitalPin::detach_all();
  platform::threads::yield_this_thread();

  // shutdown all external power
  if (not logic::power::go_to_shutdown())
  {
    // TODO: error ?
  }

  uint8_t maxChecks = 100;
  while (platform::threads::is_all_suspended() != 1 and maxChecks > 0)
  {
    maxChecks--;
    // block other threads
    platform::threads::suspend_all_threads();
    platform::threads::yield_this_thread();
    platform::delay_ms(5);
  }
  if (maxChecks == 0)
  {
    // some task have refused to power off !!
    // TODO : alert ?
  }

  // deactivate strip power
  physical::outputPower::write_voltage(0); // power down
  platform::delay_ms(10);

  // disable bluetooth, imu and microphone
  physical::microphone::disable();
  physical::imu::shutdown();
  platform::bluetooth::stop_bluetooth_advertising();

  statistics::signal_output_off();

  // save the current config to a file
  // (takes some time so call it when the lamp appear to be shutdown already)
  write_parameters(shouldSaveUserParameters, shouldSaveSystemParameters);

  // power the system off
  true_power_off();
}

uint32_t get_bluetooth_auto_activation_left() { return bluetoothAutoActivationLeftCount; }

} // namespace internal

/// Handle the behavior states
void state_machine_behavior()
{
  mainMachine.run();
  // if state changed, display the new state
  if (mainMachine.state_just_changed())
  {
    platform::lampda_print("BEHAVIOR_S_MACH > switched to state %s",
                           BehaviorStatesStr[static_cast<size_t>(mainMachine.get_state())]);
  }

  switch (mainMachine.get_state())
  {
    // strange error state
    case BehaviorStates::ERROR:
      internal::handle_error_state();
      break;
    // called when system starts
    case BehaviorStates::START_LOGIC:
      internal::handle_start_logic_state();
      break;
    // prepare the charger operation
    case BehaviorStates::PRE_CHARGER_OPERATION:
      internal::handle_pre_charger_operation_state();
      break;
    // charge batteries, or power OTG on USB-port
    case BehaviorStates::CHARGER_OPERATIONS:
      internal::handle_charger_operation_state();
      break;
    // prepare the output light
    case BehaviorStates::PRE_OUTPUT_LIGHT:
      internal::handle_pre_output_light_state();
      break;
    // output current to system, handle user inputs
    case BehaviorStates::OUTPUT_LIGHT:
      internal::handle_output_light_state();
      break;
    case BehaviorStates::POST_OUTPUT_LIGHT:
      internal::handle_post_output_light_state();
      break;
    case BehaviorStates::SHUTDOWN:
      internal::handle_shutdown_state();
      break;
    default:
      {
        go_to_error_state("reached state machine default state");
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
  const bool shouldIgnoreAlerts = (platform::time_ms() - wakeUpTime) < 500;
  logic::alerts::handle_all(shouldIgnoreAlerts);
  // alert requested an emergency shutdown, do it
  if (logic::alerts::is_request_shutdown())
  {
    platform::lampda_print("emergency shutdown from alert");
    // just in case, turn off eventual states
    internal::handle_post_output_light_state();
    // shutdown normally
    internal::handle_shutdown_state();
  }

  // only handle those when system is not in alert state
  if (mainMachine.get_state() == BehaviorStates::PRE_OUTPUT_LIGHT or
      mainMachine.get_state() == BehaviorStates::OUTPUT_LIGHT)
  {
    // at any point when the alert is raised, got to power off
    if (logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT) and
        logic::alerts::manager.get_time_since_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT) > 950)
    {
      const auto& buttonState = physical::button::get_button_state();
      const bool isButtonPressed = buttonState.isPressed or buttonState.isLongPressed;
      // shutdown with button pressed starts right back up.
      if (not isButtonPressed)
        behavior::set_power_off();
    }
  }
}

std::string get_state() { return std::string(BehaviorStatesStr[static_cast<size_t>(mainMachine.get_state())]); }

} // namespace behavior
} // namespace logic
} // namespace lampda

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
