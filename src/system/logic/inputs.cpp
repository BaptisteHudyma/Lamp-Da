#include "inputs.h"

#include "src/user/functions.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/alerts.h"

#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/time.h"

#include "src/system/power/power_handler.h"

#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/time_utils.h"
#include "src/system/utils/sunset_timer.h"

#include <cstdint>

namespace inputs {

// constants
static constexpr uint32_t BRIGHTNESS_RAMP_DURATION_MS = 2000; /// duration of the brightness ramp
static constexpr uint32_t BRIGHTNESS_LOOP_UPDATE_EVERY = 20;  /// frequency update of the ramp

// hold the boolean that configures if button's usermode UI is enabled
bool isButtonUsermodeEnabled = false;

// hold a boolean to avoid advertising several times in a row
bool isBluetoothAdvertising = false;

namespace button_press_handles {

/**
 * \brief handle system start event
 *
 * \return false if an action was handled
 */
bool system_start_button_click_callback(const uint8_t consecutiveButtonCheck)
{
  const bool isStartedInLockout = alerts::manager.is_raised(alerts::Type::SYSTEM_IN_LOCKOUT);
  // if in lockout mode, unlock
  if (consecutiveButtonCheck >= 3)
  {
    alerts::manager.clear(alerts::Type::SYSTEM_IN_LOCKOUT);
  }

  switch (consecutiveButtonCheck)
  {
    case 4:
      // activate lockout if not already in it
      if (not isStartedInLockout)
        alerts::manager.raise(alerts::Type::SYSTEM_IN_LOCKOUT);
      return false;
    default:
      break;
  }
  return true;
}

/**
 *  \brief handle custom actions
 */
bool usermode_button_click_callback(const uint8_t consecutiveButtonCheck)
{
  // user mode may return "True" to skip default action
  if (user::button_clicked_usermode(consecutiveButtonCheck))
  {
    return false;
  }
  return true;
}

/**
 * \brief button click callbacks that will ALWAYS be called
 *
 * \return true if the button handling can continue
 */
bool always_button_click_callback(const uint8_t consecutiveButtonCheck)
{
  switch (consecutiveButtonCheck)
  {
    case 7:
      {
        indicator::set_brightness_level(indicator::get_brightness_level() + 1);
        return false;
      }
    default:
      break;
  }
  return true;
}

/**
 * \brief Handle inputs when system is turned on
 */
void system_enabled_button_click_callback(const uint8_t consecutiveButtonCheck)
{
  // basic "default" UI:
  //  - 1 click: on/off
  //  - 10+ clicks: shutdown immediately (if DEBUG_MODE wait for watchdog)
  //
  switch (consecutiveButtonCheck)
  {
    // 1 click: shutdown
    case 1:
      // do not turn off on first button press
      if (not button::is_system_start_click())
      {
        behavior::set_power_off();
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
        behavior::set_power_off();
        return;
      }

      user::button_clicked_default(consecutiveButtonCheck);
      break;
  }
}

/**
 * \brief handle system start event
 *
 * \return false if an action was handled
 */
bool system_start_button_hold_callback(const uint8_t consecutiveButtonCheck,
                                       const bool isEndOfHoldEvent,
                                       const uint32_t buttonHoldDuration)
{
  // if in lockout mode, unlock
  if (consecutiveButtonCheck >= 3)
  {
    alerts::manager.clear(alerts::Type::SYSTEM_IN_LOCKOUT);
  }

  switch (consecutiveButtonCheck)
  {
    // external battery mode
    case 2:
      {
        if (buttonHoldDuration > 1000)
        {
          if (not power::is_in_otg_mode())
            behavior::go_to_external_battery_mode();
        }
        break;
      }
    case 3:
      {
        // 3+hold (2s): turn it on, with button usermode enabled
        if (buttonHoldDuration > 2000)
        {
          isButtonUsermodeEnabled = true;
        }
        return false;
      }
    case 4:
      {
#ifdef USE_BLUETOOTH
        // 4+hold (2s): turn on, with bluetooth advertising
        if (buttonHoldDuration > 2000)
        {
          if (!isBluetoothAdvertising)
          {
            bluetooth::start_advertising();
          }

          isBluetoothAdvertising = true;
        }
        return false;
#endif
      }
    default:
      break;
  }

  return true;
}

/**
 *  \brief handle custom actions
 */
bool usermode_button_hold_callback(const uint8_t consecutiveButtonCheck,
                                   const bool isEndOfHoldEvent,
                                   const uint32_t buttonHoldDuration)
{
  switch (consecutiveButtonCheck)
  {
    // 5+hold (5s): always exit, can't be bypassed
    case 5:
      {
        if (buttonHoldDuration > 5000)
        {
          behavior::set_power_off();
          return false;
        }
        // passthrought
      }
    default:
      {
        // user mode may return "True" to skip default action
        if (user::button_hold_usermode(consecutiveButtonCheck, isEndOfHoldEvent, buttonHoldDuration))
        {
          return false;
        }
        break;
      }
  }

  return true;
}

/**
 * \brief button hold callbacks that will ALWAYS be called
 *
 * \return true if the button handling can continue
 */
bool always_button_hold_callback(const uint8_t consecutiveButtonCheck,
                                 const bool isEndOfHoldEvent,
                                 const uint32_t buttonHoldDuration)
{
  switch (consecutiveButtonCheck)
  {
    // 7 clics and hold : toggle indicator
    case 7:
      {
        // every seconds, update the indicator
        EVERY_N_MILLIS(1000) { indicator::set_brightness_level(indicator::get_brightness_level() + 1); }
        return false;
      }
    default:
      break;
  }

  return true;
}

/**
 * \brief Handle inputs when system is turned on
 *
 */
void system_enabled_button_hold_callback(const uint8_t consecutiveButtonCheck,
                                         const bool isEndOfHoldEvent,
                                         const uint32_t buttonHoldDuration)
{
  // basic "default" UI:
  //
  switch (consecutiveButtonCheck)
  {
    // 1+hold, 2+hold: brightness control
    case 1:
    case 2:
      {
        // number of steps to update brightness
        static constexpr uint32_t brightnessUpdateSteps = BRIGHTNESS_RAMP_DURATION_MS / BRIGHTNESS_LOOP_UPDATE_EVERY;
        static uint32_t brightnessUpdateStepSize = max<uint32_t>(1, maxBrightness / brightnessUpdateSteps);

        // negative go low, positive go high
        static int rampSide = 1;
        static uint32_t lastBrightnessUpdateTime_ms = 0;

        if (isEndOfHoldEvent)
        {
          // reverse on release
          rampSide = -rampSide;
          lastBrightnessUpdateTime_ms = time_ms();
          brightness::update_saved_brightness();
          break;
        }

        // update brightness every N milliseconds (or end of hold)
        EVERY_N_MILLIS(BRIGHTNESS_LOOP_UPDATE_EVERY)
        {
          const brightness_t brightness = brightness::get_brightness();

          // first actions, set ramp side
          if (buttonHoldDuration <= BRIGHTNESS_LOOP_UPDATE_EVERY)
          {
            // 2 clics always go low
            if (consecutiveButtonCheck == 2)
              rampSide = -1;
            // ramp at maximum, go low
            else if (brightness >= maxBrightness)
              rampSide = -1;
            // if more than 1 second elapsed, fall back to raise ramp
            else if (lastBrightnessUpdateTime_ms == 0 or time_ms() - lastBrightnessUpdateTime_ms >= 1000)
              rampSide = 1;
          }
          // press for too long, go down
          // TODO do this from level max, no start of ramp
          else if (buttonHoldDuration >= 5000)
          {
// this do not work, because system starts right back up, because button is still pulled low...
#if 0
          // stayed pressed too long, turn off
          if (buttonHoldDuration >= 10000)
          {
            behavior::set_power_off();
            break;
          }
          else
#endif
            rampSide = -1;
          }

          // go down
          if (rampSide < 0)
          {
            if (brightness <= brightnessUpdateStepSize)
              // min level
              brightness::update_brightness(1);
            else
              brightness::update_brightness(static_cast<brightness_t>(brightness - brightnessUpdateStepSize));
          }
          /// go up
          else
          {
            // alert of sunset update
            sunset::bump_timer();

            if (brightness + brightnessUpdateStepSize >= maxBrightness)
              // min level
              brightness::update_brightness(maxBrightness);
            else
              brightness::update_brightness(static_cast<brightness_t>(brightness + brightnessUpdateStepSize));
          }

          // update saved brightness
          brightness::update_saved_brightness();
        }
      }
      break;

    // other behaviors
    default:
      user::button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, buttonHoldDuration);
      break;
  }
}

} // namespace button_press_handles

// call when the button is finally release after a chain of clicks
void button_clicked_callback(const uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  // guard blocking other actions than "turn off", if not allowed to run
  if (not behavior::can_system_allowed_to_be_powered() and not button::is_system_start_click())
  {
    if (consecutiveButtonCheck == 1)
    {
      behavior::set_power_off();
    }

    // block all actions
    return;
  }

  //
  // Handle system start click
  //
  if (button::is_system_start_click())
  {
    bool canContinue = button_press_handles::system_start_button_click_callback(consecutiveButtonCheck);
    if (not canContinue)
      return;

    const bool isStartedInLockout = alerts::manager.is_raised(alerts::Type::SYSTEM_IN_LOCKOUT);
    if (not isStartedInLockout)
    {
      canContinue = user::button_start_click_default(consecutiveButtonCheck);
      if (not canContinue)
        return;
    }
  }

  // no inputs allowed in lockout
  if (alerts::manager.is_raised(alerts::Type::SYSTEM_IN_LOCKOUT))
    return;

  //
  // extended "button usermode" bypass
  //
  if (isButtonUsermodeEnabled)
  {
    const bool canContinue = button_press_handles::usermode_button_click_callback(consecutiveButtonCheck);
    if (not canContinue)
      return;
  }

  //
  // Always enabled events
  //
  const bool canContinue = button_press_handles::always_button_click_callback(consecutiveButtonCheck);
  if (not canContinue)
    return;

  //
  //  guard blocking other actions than "turning on"
  //
  if (behavior::is_user_code_running())
  {
    // system is enabled, handle events
    button_press_handles::system_enabled_button_click_callback(consecutiveButtonCheck);
  }
  else
  {
    // only available action is turning on
    if (consecutiveButtonCheck == 1)
    {
      behavior::set_power_on();
    }
  }
}

// call when the button in a chain of clicks with a long press
// It is called every loop turn while button is pressed
void button_hold_callback(const uint8_t consecutiveButtonCheck, const uint32_t buttonHoldDuration)
{
  if (consecutiveButtonCheck == 0)
    return;

  // compute parameters of the "press-hold" action
  const bool isEndOfHoldEvent = (buttonHoldDuration <= 1);

  // rectify hold duration
  const uint32_t holdDuration =
          (buttonHoldDuration > HOLD_BUTTON_MIN_MS) ? (buttonHoldDuration - HOLD_BUTTON_MIN_MS) : 0;

  //
  // "start event" button
  //
  if (button::is_system_start_click())
  {
    bool canContinue = button_press_handles::system_start_button_hold_callback(
            consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
    if (not canContinue)
      return;

    const bool isStartedInLockout = alerts::manager.is_raised(alerts::Type::SYSTEM_IN_LOCKOUT);
    if (not isStartedInLockout)
    {
      canContinue = user::button_start_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
      if (not canContinue)
        return;
    }
  }

  // no inputs allowed in lockout
  if (alerts::manager.is_raised(alerts::Type::SYSTEM_IN_LOCKOUT))
    return;

  //
  // "button usermode" bypass
  //
  if (isButtonUsermodeEnabled)
  {
    const bool canContinue =
            button_press_handles::usermode_button_hold_callback(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
    if (not canContinue)
      return;
  }

  //
  // "button always handled" actions
  //
  const bool canContinue =
          button_press_handles::always_button_hold_callback(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
  // dont handle events if false
  if (not canContinue)
    return;

  //
  // "user code button" actions
  //
  if (behavior::is_user_code_running())
  {
    // handle user inputs
    button_press_handles::system_enabled_button_hold_callback(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
  }
}

void init(const bool wasPoweredByUserInterrupt)
{
  button::init(wasPoweredByUserInterrupt);
  indicator::init();
}

void loop()
{
  // loop is not ran in shutdown mode
  button::handle_events(button_clicked_callback, button_hold_callback);
}

void button_disable_usermode() { isButtonUsermodeEnabled = false; }

bool is_button_usermode_enabled() { return isButtonUsermodeEnabled; }

} // namespace inputs
