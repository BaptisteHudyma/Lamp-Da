#ifndef PHYSICAL_BUTTON_CPP
#define PHYSICAL_BUTTON_CPP

#include "button.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"

#include "src/system/logic/inputs.h"
#include "src/system/logic/statistics_handler.h"

#include "src/system/platform/print.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/time.h"

namespace lampda {
namespace physical {
namespace button {

// The button pullup pin (one button pin to GND, the other to this pin)
platform::gpio::DigitalPin::GPIO _buttonPin = platform::gpio::DigitalPin::GPIO::gpio3;
platform::gpio::DigitalPin _buttonGpio(_buttonPin);

platform::gpio::DigitalPin::GPIO get_button_pin() { return _buttonPin; }
int get_button_pin_RAW() { return _buttonGpio.pin(); }
void set_button_pin(const platform::gpio::DigitalPin::GPIO buttonPin) { _buttonPin = buttonPin; }

// button pressed states
static ButtonStateTy buttonState = ButtonStateTy();
ButtonStateTy get_button_state() { return buttonState; }

bool is_button_pressed()
{
  // this is a pullup, so high means no button press
  return not _buttonGpio.is_high();
}

static bool isSystemStartClick = true; ///< keep track of the first button click since waking up

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt()
{
  // There is a bit too much operations for an interrupt, but it's supposed to be rare
  // And it's ok for a real time kernel
  const bool isbuttonStillpressed = is_button_pressed();
  if (isbuttonStillpressed)
  {
    const uint32_t currentTime = platform::time_ms();
    // small delay since button press, register a click
    if ((currentTime - buttonState.lastPressTime) > RELEASE_BETWEEN_CLICKS)
    {
      if (!buttonState.isLongPressed)
      {
        buttonState.nbClicksCounted += 1;
        buttonState.firstHoldTime = currentTime;

        // stat update
        logic::statistics::signal_button_press();
      }
    }
    // update trigger flags
    buttonState.lastPressTime = currentTime;
    buttonState.wasTriggered = true;
    buttonState.isPressed = true;
  }

  // update flag
  wasButtonPressedDetected = isbuttonStillpressed;
}

void handle_events()
{
  const bool isButtonPressDetected = wasButtonPressedDetected;
  const uint32_t currentTime = platform::time_ms();
  const uint32_t sinceLastCall = currentTime - buttonState.lastPressTime;
  const uint32_t pressDuration = currentTime - buttonState.firstHoldTime;

  // currently in long press status
  buttonState.isLongPressed = (buttonState.isPressed and pressDuration > HOLD_BUTTON_MIN_MS);

  // remove button clicked if last call was too long ago (and an action is currently handled)
  if (buttonState.wasTriggered and
      ((sinceLastCall > RELEASE_TIMING_MS) or (buttonState.isLongPressed and sinceLastCall > RELEASE_TIMING_MS / 2)))
  {
    // end of button press, trigger callback (press-hold action, or press action)
    if (buttonState.isLongPressed)
    {
      const bool isSuccess = logic::inputs::add_button_press_event(buttonState.nbClicksCounted, 0, isSystemStartClick);
      if (not isSuccess)
      {
        platform::lampda_print("Button: Could not register end of hold event");
      }
    }
    else
    {
      const bool isSuccess = logic::inputs::add_button_click_event(buttonState.nbClicksCounted, isSystemStartClick);
      if (not isSuccess)
      {
        platform::lampda_print("Button: Could not register end of click event, droping less important events");
      }
    }

    // reset
    isSystemStartClick = false;

    buttonState.isPressed = false;
    buttonState.isLongPressed = false;
    buttonState.nbClicksCounted = 0;
    buttonState.firstHoldTime = currentTime;
    // reset the action handling process
    buttonState.wasTriggered = false;
  }

  // set button high
  if (isButtonPressDetected)
  {
    // press detected, update last event time
    buttonState.lastPressTime = currentTime;
  }

  if (buttonState.isLongPressed)
  {
    // press detected, trigger
    buttonState.wasTriggered = true;

    const bool isSuccess =
            logic::inputs::add_button_press_event(buttonState.nbClicksCounted, pressDuration, isSystemStartClick);
    if (not isSuccess)
    {
      platform::lampda_print("Button: Could not register hold event");
    }
  }

  // safety : an interrupt may have been missed, and the button is locked in a logic pressed state
  if (isButtonPressDetected && not is_button_pressed() && pressDuration > RELEASE_BETWEEN_CLICKS)
  {
    button_state_interrupt();
    platform::lampda_print("Button interrupt shortcut due to state lock detected");
  }
}

void button_thread()
{
  handle_events();

  platform::delay_ms(thread_throttle_time_ms);
}

void init(const bool isSystemStartedFromButton)
{
  static_assert(RELEASE_BETWEEN_CLICKS < RELEASE_TIMING_MS,
                "release debounce should always be less than release timing");
  static_assert((RELEASE_BETWEEN_CLICKS + RELEASE_TIMING_MS) < HOLD_BUTTON_MIN_MS,
                "button release timing should always be less then the button hold timing");

  // if button if already started, reset it
  isSystemStartClick = true;
  buttonState.reset();

  // attach the button interrupt
  _buttonGpio.set(_buttonPin);
  _buttonGpio.set_pin_mode(platform::gpio::DigitalPin::Mode::kInputPullUpSense);
  _buttonGpio.attach_callback(button_state_interrupt, platform::gpio::DigitalPin::Interrupt::kChange);

  // prevent multiple clicks on start
  if (isSystemStartedFromButton and buttonState.nbClicksCounted == 0)
  {
    // simulate a click
    wasButtonPressedDetected = true;
    buttonState.nbClicksCounted = 1;
    buttonState.lastPressTime = platform::time_ms();
    buttonState.firstHoldTime = platform::time_ms();
    buttonState.wasTriggered = true;
    buttonState.isPressed = true;

    logic::statistics::signal_button_press();
  }
  else
  {
    isSystemStartClick = false;
  }

  platform::threads::start_thread(button_thread, platform::threads::button_taskName, 2, 255);
}

} // namespace button
} // namespace physical
} // namespace lampda

#endif
