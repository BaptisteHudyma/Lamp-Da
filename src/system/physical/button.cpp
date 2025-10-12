#ifndef PHYSICAL_BUTTON_CPP
#define PHYSICAL_BUTTON_CPP

#include "button.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"

#include "src/system/logic/statistics_handler.h"

#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

namespace button {

static ButtonStateTy buttonState = ButtonStateTy();
ButtonStateTy get_button_state() { return buttonState; }

bool is_button_pressed()
{
  // this is a pullup, so high means no button press
  return not ButtonPin.is_high();
}

static bool isSystemStartClick = true;

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt()
{
  // There is a bit too much operations for an interrupt, but it's supposed to be rare
  // And it's ok for a real time kernel
  const bool isbuttonStillpressed = is_button_pressed();
  if (isbuttonStillpressed)
  {
    const uint32_t currentTime = time_ms();
    // small delay since button press, register a click
    if ((currentTime - buttonState.lastPressTime) > RELEASE_BETWEEN_CLICKS)
    {
      if (!buttonState.isLongPressed)
      {
        buttonState.nbClicksCounted += 1;
        buttonState.firstHoldTime = currentTime;

        // stat update
        statistics::signal_button_press();
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

void init(const bool isSystemStartedFromButton)
{
  static_assert(RELEASE_BETWEEN_CLICKS < RELEASE_TIMING_MS,
                "release debounce should always be less than release timing");
  static_assert((RELEASE_BETWEEN_CLICKS + RELEASE_TIMING_MS) < HOLD_BUTTON_MIN_MS,
                "button release timing should always be less then the button hold timing");

  // attach the button interrupt
  ButtonPin.set_pin_mode(DigitalPin::Mode::kInputPullUpSense);
  ButtonPin.attach_callback(button_state_interrupt, DigitalPin::Interrupt::kChange);

  // prevent multiple clicks on start
  if (isSystemStartedFromButton and buttonState.nbClicksCounted == 0)
  {
    // simulate a click
    wasButtonPressedDetected = true;
    buttonState.nbClicksCounted = 1;
    buttonState.lastPressTime = time_ms();
    buttonState.firstHoldTime = time_ms();
    buttonState.wasTriggered = true;
    buttonState.isPressed = true;

    statistics::signal_button_press();
  }
  else
  {
    isSystemStartClick = false;
  }
}

void handle_events(const std::function<void(uint8_t)>& clickSerieCallback,
                   const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback)
{
  const bool isButtonPressDetected = wasButtonPressedDetected;
  const uint32_t currentTime = time_ms();
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
      clickHoldSerieCallback(buttonState.nbClicksCounted, 0);
    }
    else
    {
      clickSerieCallback(buttonState.nbClicksCounted);
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

    clickHoldSerieCallback(buttonState.nbClicksCounted, pressDuration);
  }

  // safety : an interrupt may have been missed, and the button is locked in a logic pressed state
  if (isButtonPressDetected && not is_button_pressed() && pressDuration > RELEASE_BETWEEN_CLICKS)
  {
    button_state_interrupt();
    lampda_print("Button interrupt shortcut due to state lock detected");
  }
}

bool is_system_start_click() { return isSystemStartClick; }

} // namespace button

#endif
