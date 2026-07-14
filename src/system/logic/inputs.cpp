#include "inputs.h"

#include "src/user/functions.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/time_utils.h"

#include <cstdint>
namespace lampda {
namespace logic {
namespace inputs {

// constants
static constexpr uint32_t BRIGHTNESS_RAMP_DURATION_MS = 2000; ///< duration of the brightness ramp
static constexpr uint32_t BRIGHTNESS_LOOP_UPDATE_EVERY = 20;  ///< frequency update of the ramp
static constexpr uint32_t BRIGHTNESS_RAMP_SATURATION_MAX_DURATION_MS =
        5000; ///< duration of the brightness ramp saturation before switching behavior

namespace __private {
utils::Queue<ButtonEvent, maxButtonEventStore> buttonEventQueue; ///< button event asynchroneous queue
}

/// holds the state of system on
bool isSystemOn = false;
/// hold the boolean that configures if button's usermode UI is enabled
bool isButtonUsermodeEnabled = false;

namespace button_press_handles {

/**
 * \brief handle system start event
 *
 * \return false if an action was handled
 */
bool system_start_button_click_callback(const uint8_t consecutiveButtonCheck)
{
  const bool isStartedInLockout = logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
  // if in lockout mode, unlock
  if (consecutiveButtonCheck >= 3)
  {
    logic::alerts::manager.clear(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
  }

  switch (consecutiveButtonCheck)
  {
    case 4:
      {
        // activate lockout if not already in it
        if (not isStartedInLockout)
        {
          logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
          return false;
        }
        break;
      }
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
  const bool canContinue = not user::button_clicked_usermode(consecutiveButtonCheck);
  if (not canContinue)
  {
    // nothing to do, in custom usermode, system is already on
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
        indicator::set_brightness_level(logic::indicator::get_brightness_level() + 1);
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
void system_enabled_button_click_callback(const uint8_t consecutiveButtonCheck, const bool isFirstClick)
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
      if (not isFirstClick)
      {
        behavior::set_power_off();
      }
      break;

    // other behaviors
    default:
      {
        // 10+ clicks: force shutdown (or safety reset if DEBUG_MODE)
        if (consecutiveButtonCheck >= 10)
        {
#ifdef DEBUG_MODE
          // disable charger and wait 5s to be killed by watchdog
          indicator::set_color(utils::ColorSpace::PINK);
          logic::power::enable_charge(false);
          platform::delay_ms(20000); // crash the system
#endif
          behavior::set_power_off();
          return;
        }

        const bool canContinue = not user::button_clicked_default(consecutiveButtonCheck);
        if (not canContinue)
        {
          // Nothing to do, system is already on
          return;
        }

        // other actions
        break;
      }
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
    logic::alerts::manager.clear(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
  }

  switch (consecutiveButtonCheck)
  {
    case 2:
      {
        // 2+hold (1s): external battery mode
        if (buttonHoldDuration > 1000 and not logic::power::is_in_otg_mode())
        {
          behavior::go_to_external_battery_mode();
        }
        return false;
      }
    case 3:
      {
        // 3+hold (2s): turn it on, with button usermode enabled
        if (not isButtonUsermodeEnabled and buttonHoldDuration > 2000)
        {
          behavior::set_power_on();
          isButtonUsermodeEnabled = true;
          platform::lampda_print("Wake up from start usermode command");
        }
        return false;
      }
    case 4:
      {
        // 4+hold (2s): turn on, with bluetooth advertising
        if (buttonHoldDuration > 2000)
        {
          platform::bluetooth::start_advertising();
        }
        return false;
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
        const bool hasUserHandledAction =
                user::button_hold_usermode(consecutiveButtonCheck, isEndOfHoldEvent, buttonHoldDuration);
        if (hasUserHandledAction)
        {
          // user mode should be active when system is on, no need to start again
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
        EVERY_N_MILLIS(1000) { indicator::set_brightness_level(logic::indicator::get_brightness_level() + 1); }
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
        static const uint32_t brightnessUpdateStepSize =
                max<uint32_t>(1, ::lampda::brightness::absoluteMaximumBrightness / brightnessUpdateSteps);

        // negative go low, positive go high
        static int rampSide = 1;
        static uint32_t lastBrightnessUpdateTime_ms = 0;

        // prevent sunset updates when we are updating it ourself
        logic::sunset::lock_brightness_update(not isEndOfHoldEvent);

        if (isEndOfHoldEvent)
        {
          // update logic
          lastBrightnessUpdateTime_ms = platform::time_ms();
          logic::brightness::update_saved_brightness();

          // if user set the brightness up, alert of sunset update
          if (rampSide > 0)
            logic::sunset::bump_timer();

          // reverse ramp on release
          rampSide = -rampSide;
          break;
        }

        // update brightness every N milliseconds (or end of hold)
        EVERY_N_MILLIS(BRIGHTNESS_LOOP_UPDATE_EVERY)
        {
          // use minimum of saved vs max user brightness
          const brightness_t brightness = std::min<brightness_t>(logic::brightness::get_saved_brightness(),
                                                                 logic::brightness::get_max_user_brightness());

          // first actions, set ramp side
          if (buttonHoldDuration <= BRIGHTNESS_LOOP_UPDATE_EVERY)
          {
            // 2 clics always go low
            if (consecutiveButtonCheck == 2)
              rampSide = -1;
            // ramp at maximum, go low
            else if (brightness >= logic::brightness::get_max_brightness())
              rampSide = -1;
            // if more than 1 second elapsed, fall back to raise ramp
            else if (lastBrightnessUpdateTime_ms == 0 or platform::time_ms() - lastBrightnessUpdateTime_ms >= 1000)
              rampSide = 1;
          }
          // press for too long, go down
          // TODO do this from level max, no start of ramp
          else if (buttonHoldDuration >= BRIGHTNESS_RAMP_SATURATION_MAX_DURATION_MS)
          {
            // stayed pressed too long, turn off
            if (buttonHoldDuration >= 2 * BRIGHTNESS_RAMP_SATURATION_MAX_DURATION_MS)
            {
              behavior::set_power_off();
              break;
            }
            else
              rampSide = -1;
          }

          // go down
          if (rampSide < 0)
          {
            if (brightness <= brightnessUpdateStepSize)
              // min level
              logic::brightness::update_brightness(1);
            else
              logic::brightness::update_brightness(static_cast<brightness_t>(brightness - brightnessUpdateStepSize));
          }
          /// go up
          else
          {
            // limit max brightness
            const auto _maxBrightness = logic::brightness::get_max_brightness();
            if (brightness + brightnessUpdateStepSize >= _maxBrightness)
              logic::brightness::update_brightness(_maxBrightness);
            else
              logic::brightness::update_brightness(static_cast<brightness_t>(brightness + brightnessUpdateStepSize));
          }

          // update saved brightness
          logic::brightness::update_saved_brightness();
        }
      }
      break;

    // other behaviors
    default:
      const bool hasUserHandledAction =
              user::button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, buttonHoldDuration);
      if (hasUserHandledAction)
      {
        // system is already on, no need to continue
        return;
      }
      break;
  }
}

} // namespace button_press_handles

// call when the button is finally release after a chain of clicks
void button_clicked_callback(const uint8_t consecutiveButtonCheck, const bool isFirstClick)
{
  if (consecutiveButtonCheck == 0)
    return;

  // guard blocking other actions than "turn off", if not allowed to run
  if (not behavior::can_system_allowed_to_be_powered() and not isFirstClick)
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
  if (isFirstClick)
  {
    bool canContinue = button_press_handles::system_start_button_click_callback(consecutiveButtonCheck);
    if (not canContinue)
      return;

    const bool isStartedInLockout = logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
    if (not isStartedInLockout)
    {
      // user mode may return "True" to skip default action
      canContinue = not user::button_start_click_default(consecutiveButtonCheck);
      if (not canContinue)
      {
        // user returned a command handle, so we should start
        behavior::set_power_on();
        platform::lampda_print("Wake up from start user::clicks %d", consecutiveButtonCheck);
        return;
      }
    }
  }

  // no inputs allowed in lockout
  if (logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT))
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
    button_press_handles::system_enabled_button_click_callback(consecutiveButtonCheck, isFirstClick);
  }
  else
  {
    // only available action is turning on
    if (consecutiveButtonCheck == 1)
    {
      behavior::set_power_on();
      platform::lampda_print("Wake up from single press");
    }
  }
}

// call when the button in a chain of clicks with a long press
// It is called every loop turn while button is pressed
void button_hold_callback(const uint8_t consecutiveButtonCheck,
                          const uint32_t buttonHoldDuration,
                          const bool isEndOfPress,
                          const bool isFirstClick)
{
  if (consecutiveButtonCheck == 0)
    return;

  // rectify hold duration
  const uint32_t holdDuration = (buttonHoldDuration > physical::button::HOLD_BUTTON_MIN_MS) ?
                                        (buttonHoldDuration - physical::button::HOLD_BUTTON_MIN_MS) :
                                        0;

  //
  // "start event" button
  //
  if (isFirstClick)
  {
    bool canContinue =
            button_press_handles::system_start_button_hold_callback(consecutiveButtonCheck, isEndOfPress, holdDuration);
    if (not canContinue)
      return;

    const bool isStartedInLockout = logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
    if (not isStartedInLockout)
    {
      // user mode may return "True" to skip default action
      canContinue = not user::button_start_hold_default(consecutiveButtonCheck, isEndOfPress, holdDuration);
      if (not canContinue)
      {
        if (not behavior::is_system_should_be_powered())
        {
          // user returned a command handle, so we should start
          behavior::set_power_on();
          platform::lampda_print("Wake up from start user::hold %d", consecutiveButtonCheck);
        }
        return;
      }
    }
  }

  // no inputs allowed in lockout
  if (logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT))
    return;

  //
  // "button usermode" bypass
  //
  if (isButtonUsermodeEnabled)
  {
    const bool canContinue =
            button_press_handles::usermode_button_hold_callback(consecutiveButtonCheck, isEndOfPress, holdDuration);
    if (not canContinue)
      return;
  }

  //
  // "button always handled" actions
  //
  const bool canContinue =
          button_press_handles::always_button_hold_callback(consecutiveButtonCheck, isEndOfPress, holdDuration);
  // dont handle events if false
  if (not canContinue)
    return;

  //
  // "user code button" actions
  //
  if (behavior::is_user_code_running())
  {
    // handle user inputs
    button_press_handles::system_enabled_button_hold_callback(consecutiveButtonCheck, isEndOfPress, holdDuration);
  }
}

void init(const bool wasPoweredByUserInterrupt)
{
  physical::button::init(wasPoweredByUserInterrupt);
  physical::indicator::init();
}

void loop()
{
  // loop is not ran in shutdown mode

  // Check a maximum of events per loop turn, events can be refilled by other threads
  size_t maxEventsChecks = __private::maxButtonEventStore;
  while (maxEventsChecks != 0 && __private::buttonEventQueue.has_elements())
  {
    maxEventsChecks--;
    const auto& event = __private::buttonEventQueue.dequeue();
    if (event.has_value())
    {
      const __private::ButtonEvent& buttonEvent = event.value();
      if (not buttonEvent.isLongPress)
      {
        button_clicked_callback(buttonEvent.clickCount, not isSystemOn);
      }
      else
      {
        button_hold_callback(
                buttonEvent.clickCount, buttonEvent.longPressDuration, buttonEvent.isEndOfLongPress, not isSystemOn);
      }

      // update system on
      if (not buttonEvent.isLongPress or buttonEvent.isEndOfLongPress)
      {
        isSystemOn = behavior::is_system_should_be_powered();
        // deactivate custom user mode on turn off
        if (not isSystemOn)
          button_disable_usermode();
      }
    }
  }
}

void button_disable_usermode() { isButtonUsermodeEnabled = false; }

bool is_button_usermode_enabled() { return isButtonUsermodeEnabled; }

bool add_button_click_event(uint32_t clickCount)
{
  __private::ButtonEvent event;
  event.isLongPress = false;
  event.clickCount = clickCount;
  return __private::buttonEventQueue.enqueue(event);
}

bool add_button_press_event(uint32_t clickCount, uint32_t pressDuration, bool isEndOfPress)
{
  __private::ButtonEvent event;
  event.isLongPress = true;
  event.clickCount = clickCount;
  event.isEndOfLongPress = isEndOfPress;
  event.longPressDuration = pressDuration;
  return __private::buttonEventQueue.enqueue(event);
}

} // namespace inputs
} // namespace logic
} // namespace lampda
