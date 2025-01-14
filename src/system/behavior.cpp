#include "behavior.h"

#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/system/alerts.h"
#include "src/system/charger/charger.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"
#include "src/system/physical/IMU.h"
#include "src/system/physical/led_power.h"

#include "src/system/utils/colorspace.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/state_machine.h"
#include "src/system/utils/input_output.h"

#include "src/system/platform/MicroPhone.h"
#include "src/system/platform/bluetooth.h"
#include "src/system/platform/fileSystem.h"
#include "src/system/platform/time.h"
#include "src/system/platform/print.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/registers.h"

#include "src/user/functions.h"

namespace behavior {

static constexpr uint32_t brightnessKey = utils::hash("brightness");

// constants
static constexpr uint8_t MIN_BRIGHTNESS = 5;
static constexpr uint8_t MAX_BRIGHTNESS = 255;
static constexpr uint32_t BRIGHTNESS_RAMP_DURATION_MS = 2000;
static constexpr uint32_t EARLY_ACTIONS_LIMIT_MS = 2000;
static constexpr uint32_t EARLY_ACTIONS_HOLD_MS = 1500;

// Define the state for the main prog state machine
typedef enum
{
  // handle the start logic
  START_LOGIC,

  // Prepare the charging operations
  PRE_CHARGER_OPERATION,
  // Chargering, or usb OTG mode
  CHARGER_OPERATIONS,
  // close the charger components
  POST_CHARGER_OPERATIONS,
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
        "POST_CHARGER_OPERATIONS",
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
void set_power_off() { isTargetPoweredOn_s = false; }
static bool isShutingDown_s = false;

// return true if vbus is high
bool is_charger_powered() { return charger::is_vbus_powered(); }

// temporary upper bound for the brightness
static uint8_t MaxBrightnessLimit = MAX_BRIGHTNESS;

// hold the last time startup_sequence has been called
uint32_t lastStartupSequence = 0;

// hold the boolean that configures if button's usermode UI is enabled
bool isButtonUsermodeEnabled = false;

// hold a boolean to avoid advertising several times in a row
bool isBluetoothAdvertising = false;

// hold the current level of brightness out of the raise/lower animation
uint8_t BRIGHTNESS = 50; // default start value
uint8_t currentBrightness = 50;

// timestamp of the system wake up
static uint32_t turnOnTime = time_ms();

void update_brightness(const uint8_t newBrightness, const bool shouldUpdateCurrentBrightness, const bool isInitialRead)
{
  // safety
  if (newBrightness > MaxBrightnessLimit)
    return;

  if (shouldUpdateCurrentBrightness)
    currentBrightness = newBrightness;

  if (BRIGHTNESS != newBrightness)
  {
    BRIGHTNESS = newBrightness;

    // do not call user functions when reading parameters
    if (!isInitialRead)
    {
      user::brightness_update(newBrightness);
    }
  }
}

void read_parameters()
{
  // load values in memory
  fileSystem::load_initial_values();

  uint32_t brightness = 0;
  if (fileSystem::get_value(brightnessKey, brightness))
  {
    update_brightness(brightness, true, true);
  }

  user::read_parameters();
}

void write_parameters()
{
  fileSystem::clear();

  fileSystem::set_value(isFirstBootKey, 0);
  fileSystem::set_value(brightnessKey, BRIGHTNESS);

  user::write_parameters();

  fileSystem::write_state();
}

// user code is running when state is output
bool is_user_code_running() { return mainMachine.get_state() == BehaviorStates::OUTPUT_LIGHT; }

/**
 * This turns off the system FOR REAL and enable the interrupt pin for power on
 * DO NOT USE THIS IF YOU ARE NOT SURE, great potential for system brick
 *
 */
void true_power_off()
{
  charger::shutdown();
  DigitalPin(DigitalPin::GPIO::usb33Power).set_high(false);

  // wait until vbus is off (TODO: remove in newer versions of the hardware)
  uint8_t cnt = 0;
  while (cnt < 200 and charger::is_vbus_signal_detected())
  {
    delay_ms(5);
    cnt++;
  }

  // power down nrf52.
  // on wake up, it'll start back from the setup phase
  go_to_sleep(ButtonPin.pin());
  /*
   * Nothing after this, systel is off !
   */
}

void button_disable_usermode() { isButtonUsermodeEnabled = false; }

bool is_button_usermode_enabled() { return isButtonUsermodeEnabled; }

// call when the button is finally release
void button_clicked_callback(const uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  // guard blocking other actions than "turning it on"
  if (not is_user_code_running())
  {
    if (consecutiveButtonCheck == 1)
    {
      set_power_on();
    }
    return;
  }

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
  //  - 7+ clicks: shutdown immediately (if DEBUG_MODE wait for watchdog)
  //
  switch (consecutiveButtonCheck)
  {
    // 1 click: shutdown
    case 1:
      set_power_off();
      break;

    // other behaviors
    default:

      // 7+ clicks: force shutdown (or safety reset if DEBUG_MODE)
      if (consecutiveButtonCheck >= 7)
      {
#ifdef DEBUG_MODE
        // disable charger and wait 5s to be killed by watchdog
        indicator::set_color(utils::ColorSpace::PINK);
        charger::disable_charge();
        delay_ms(6000);
#endif
        set_power_off();
        return;
      }

      user::button_clicked_default(consecutiveButtonCheck);
      break;
  }
}

static constexpr float brightnessDivider = 1.0 / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

void button_hold_callback(const uint8_t consecutiveButtonCheck, const uint32_t buttonHoldDuration)
{
  if (consecutiveButtonCheck == 0)
    return;
  // no button callback when user code is not running
  if (not is_user_code_running())
    return;

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

  if (realStartTime < EARLY_ACTIONS_LIMIT_MS)
  {
    // early action animation
    if (consecutiveButtonCheck > 2)
    {
      if ((holdDuration >> 7) & 0b1)
      {
        indicator::set_color(utils::ColorSpace::BLACK);
      }
      else if (holdDuration < EARLY_ACTIONS_HOLD_MS)
      {
        indicator::set_color(utils::ColorSpace::GREEN);
      }
      else if (consecutiveButtonCheck == 3)
      {
        indicator::set_color(utils::ColorSpace::YELLOW);
      }
      else if (consecutiveButtonCheck == 4)
      {
        indicator::set_color(utils::ColorSpace::BLUE);
      }
    }

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
      else
      {
        isBluetoothAdvertising = false;
      }
    }
#endif

    // during "early actions" prevent other actions
    if (consecutiveButtonCheck > 2)
    {
      return;
    }
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

  // basic "default" UI:
  //  - 1+hold: increase brightness
  //  - 2+hold: decrease brightness
  //
  switch (consecutiveButtonCheck)
  {
    // 1+hold: increase brightness
    case 1:
      if (isEndOfHoldEvent)
      {
        // this action is duplicated, but it's rare so no consequences
        update_brightness(BRIGHTNESS, true);
      }
      else
      {
        const float percentOfTimeToGoUp = float(MAX_BRIGHTNESS - currentBrightness) * brightnessDivider;

        const auto newBrightness = utils::map(min(holdDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp),
                                              0,
                                              BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
                                              currentBrightness,
                                              MAX_BRIGHTNESS);

        update_brightness(newBrightness);
      }
      break;

    // 2+hold: decrease brightness
    case 2:
      if (isEndOfHoldEvent)
      {
        // this action is duplicated, but it's rare so no consequences
        update_brightness(BRIGHTNESS, true);
      }
      else
      {
        const double percentOfTimeToGoDown = float(currentBrightness - MIN_BRIGHTNESS) * brightnessDivider;

        const auto newBrightness = utils::map(min(holdDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown),
                                              0,
                                              BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown,
                                              currentBrightness,
                                              MIN_BRIGHTNESS);

        update_brightness(newBrightness);
      }
      break;

    // other behaviors
    default:
      user::button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
      break;
  }
}

void set_cpu_temperarure_alerts()
{
  // temperature alerts
  const float procTemp = read_CPU_temperature_degreesC();
  if (procTemp >= criticalSystemTemp_c)
  {
    AlertManager.raise_alert(TEMP_CRITICAL);
  }
  else if (procTemp >= maxSystemTemp_c)
  {
    AlertManager.raise_alert(TEMP_TOO_HIGH);
  }
  else
  {
    AlertManager.clear_alert(TEMP_TOO_HIGH);
    AlertManager.clear_alert(TEMP_CRITICAL);
  }
}

void set_battery_alerts()
{
  const auto& chargerState = charger::get_state();
  if (chargerState.status == charger::Charger_t::ChargerStatus_t::ERROR_BATTERY_MISSING)
  {
    // TODO: alert that the battery is missing
  }
  else if (chargerState.is_charging())
  {
    // clear alerts when the device is charging
    AlertManager.clear_alert(Alerts::BATTERY_LOW);
    AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);
  }
  else
  {
    // no battery alert when charger is on
    battery::raise_battery_alert();
  }
}

void handle_alerts()
{
  // do not display alerts for the first 500 ms
  const uint32_t current = ((time_ms() - turnOnTime) < 500) ? Alerts::NONE : AlertManager.current();

  static uint32_t criticalbatteryRaisedTime = 0;
  if (current == Alerts::NONE)
  {
    MaxBrightnessLimit = MAX_BRIGHTNESS; // no alerts: reset the max brightness
    criticalbatteryRaisedTime = 0;

    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                                        utils::ColorSpace::GREEN.get_rgb().color,
                                                                        battery::get_raw_battery_level() / 10000.0));

    // display battery level
    const auto& chargerStatus = charger::get_state();
    if (chargerStatus.is_charging())
    {
      // power detected with no charge or slow charging raises a special animation
      if (chargerStatus.status == charger::Charger_t::ChargerStatus_t::POWER_DETECTED or
          chargerStatus.status == charger::Charger_t::ChargerStatus_t::SLOW_CHARGING)
      {
        // fast blinking
        // TODO: find a better way to tell user that the chargeur is bad
        indicator::blink(500, 500, buttonColor);
      }
      // standard charge mode
      else
      {
        indicator::breeze(2000, 1000, buttonColor);
      }
    }
    else
    {
      // normal mode
      indicator::set_color(buttonColor);
    }
  }
  else
  {
    if ((current & Alerts::TEMP_CRITICAL) != 0x00)
    {
      // fast shutdown when temperature reaches critical levels
      mainMachine.set_state(BehaviorStates::SHUTDOWN);
    }
    else if ((current & Alerts::HARDWARE_ALERT) != 0x00)
    {
      indicator::blink(100, 50, utils::ColorSpace::TOMATO);
    }
    else if ((current & Alerts::TEMP_TOO_HIGH) != 0x00)
    {
      // proc temperature is too high, blink orange
      indicator::blink(300, 300, utils::ColorSpace::ORANGE);

      // limit brightness to half the max value
      constexpr uint8_t clampedBrightness = 0.5 * MAX_BRIGHTNESS;
      MaxBrightnessLimit = clampedBrightness;
      update_brightness(min(clampedBrightness, BRIGHTNESS), true);
    }
    else if ((current & Alerts::BATTERY_READINGS_INCOHERENT) != 0x00)
    {
      // incohrent battery readings
      indicator::blink(100, 100, utils::ColorSpace::GREEN);
    }
    else if ((current & Alerts::BATTERY_CRITICAL) != 0x00)
    {
      // critical battery alert: shutdown after 2 seconds
      if (criticalbatteryRaisedTime == 0)
        criticalbatteryRaisedTime = time_ms();
      else if (time_ms() - criticalbatteryRaisedTime > 2000)
      {
        // shutdown when battery is critical
        set_power_off();
      }
      // blink if no shutdown
      indicator::blink(100, 100, utils::ColorSpace::RED);
    }
    else if ((current & Alerts::BATTERY_LOW) != 0x00)
    {
      criticalbatteryRaisedTime = 0;
      // fast blink red
      indicator::blink(300, 300, utils::ColorSpace::RED);

      // limit brightness to quarter of the max value
      constexpr uint8_t clampedBrightness = 0.25 * MAX_BRIGHTNESS;
      MaxBrightnessLimit = clampedBrightness;

      // save some battery
      bluetooth::disable_bluetooth();
      update_brightness(min(clampedBrightness, BRIGHTNESS), true);
    }
    else if ((current & Alerts::HARDWARE_ALERT) != 0x00)
    {
      indicator::blink(100, 50, utils::ColorSpace::TOMATO);
    }
    else if ((current & Alerts::LONG_LOOP_UPDATE) != 0x00)
    {
      // fast blink red
      indicator::blink(400, 400, utils::ColorSpace::FUSHIA);
    }
    else if ((current & Alerts::BLUETOOTH_ADVERT) != 0x00)
    {
      indicator::breeze(1000, 500, utils::ColorSpace::BLUE);
    }
    else if ((current & Alerts::OTG_FAILED) != 0x00)
    {
      indicator::blink(200, 200, utils::ColorSpace::FUSHIA);
    }
    else if ((current & Alerts::OTG_ACTIVATED) != 0x00)
    {
      // red to green
      const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                                          utils::ColorSpace::GREEN.get_rgb().color,
                                                                          battery::get_raw_battery_level() / 10000.0));

      indicator::breeze(500, 500, buttonColor);
    }
    else
    {
      // unhandled case (white blink)
      indicator::blink(300, 300, utils::ColorSpace::WHITE);
    }
  }
}

void handle_error_state()
{
  // TODO ?
  // go to sleep
  mainMachine.set_state(BehaviorStates::SHUTDOWN);
}

void handle_start_logic_state()
{
  // preven early charging
  charger::set_enable_charge(false);

  if (did_woke_up_from_power())
  {
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
  preChargeCalled = time_ms();
  mainMachine.set_state(BehaviorStates::CHARGER_OPERATIONS);
}

void handle_charger_operation_state()
{
  // pressed the start button, stop charge and start lamp
  if (is_system_should_be_powered())
  {
    // switch to output mode after the post charge operations
    mainMachine.set_state(BehaviorStates::POST_CHARGER_OPERATIONS, 100, BehaviorStates::PRE_OUTPUT_LIGHT);
    return;
  }
  // power disconected
  const bool vbusDebounced = time_ms() - preChargeCalled > 500;
  if (vbusDebounced)
  {
    if (not is_charger_powered())
    {
      // go to sleep after closing the charger
      mainMachine.set_state(BehaviorStates::POST_CHARGER_OPERATIONS, 100, BehaviorStates::SHUTDOWN);
      return;
    }
    else
    {
      // enable charge after the debounce
      charger::set_enable_charge(true);
    }
  }
  // else: ignore all
}

void handle_post_charger_operation_state()
{
  // forbid charging
  charger::set_enable_charge(false);
}

void handle_pre_output_light_state()
{
  // critical battery level, do not wake up
  if (battery::get_battery_level() <= batteryCritical + 1)
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

  // button usermode is always disabled by default
  button_disable_usermode();

  // reset lastStartupSequence
  lastStartupSequence = time_ms();

  // let the user power on the system
  // TODO: this 12v activation should disapear
  ledpower::activate_12v_power();
  user::power_on_sequence();

  mainMachine.set_state(BehaviorStates::OUTPUT_LIGHT);
}

void handle_output_light_state()
{
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

    // set alerts if needed
    set_cpu_temperarure_alerts();
    set_battery_alerts();

#ifdef USE_BLUETOOTH
    bluetooth::parse_messages();
#endif
  }
}

void handle_post_output_light_state()
{
  // button usermode is kept disabled
  button_disable_usermode();

  // let the user power off the system
  user::power_off_sequence();

  // TODO: this 12v activation should disapear
  ledpower::deactivate_12v_power();

  // deactivate strip power
  ledpower::write_current(0); // power down

  mainMachine.skip_timeout();
}

void handle_shutdown_state()
{
  isShutingDown_s = true;
  // let other thread do stuff
  yield_this_thread();

  // deactivate strip power
  ledpower::write_current(0); // power down
  // TODO: this 12v activation should disapear
  ledpower::deactivate_12v_power();
  delay_ms(10);

  // disable bluetooth, imu and microphone
  microphone::disable();
  imu::disable();
#ifdef USE_BLUETOOTH
  bluetooth::disable_bluetooth();
#endif

  // save the current config to a file
  // (takes some time so call it when the lamp appear to be shutdown already)
  write_parameters();

  // deactivate indicators
  indicator::set_color(utils::ColorSpace::BLACK);

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
    case BehaviorStates::POST_CHARGER_OPERATIONS:
      handle_post_charger_operation_state();
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
  handle_alerts();
}

bool is_shuting_down() { return isShutingDown_s; }

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