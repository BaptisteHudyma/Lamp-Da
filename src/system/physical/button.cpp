#ifndef PHYSICAL_BUTTON_CPP
#define PHYSICAL_BUTTON_CPP

#include "button.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"

#include "src/system/physical/indicator.h"

#include "src/system/platform/time.h"

#define RELEASE_TIMING_MS      200
#define RELEASE_BETWEEN_CLICKS 50

namespace button {

static ButtonStateTy buttonState = ButtonStateTy();
ButtonStateTy get_button_state() { return buttonState; }

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt() { wasButtonPressedDetected = true; }

void init()
{
  // attach the button interrupt
  ButtonPin.set_pin_mode(DigitalPin::Mode::kInputPullUp);
  ButtonPin.attach_callback(button_state_interrupt, DigitalPin::Interrupt::kChange);
}

static volatile bool buttonPressListener = false;
void read_while_pressed()
{
  // this is a pullup, so high means no button press
  if (wasButtonPressedDetected and ButtonPin.is_high())
  {
    wasButtonPressedDetected = false;
  }
}

void treat_button_pressed(const bool isButtonPressDetected,
                          const std::function<void(uint8_t)>& clickSerieCallback,
                          const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback)
{
  buttonState.lastEventTime = time_ms();
  buttonState.sinceLastCall = buttonState.lastEventTime - buttonState.lastPressTime;
  buttonState.pressDuration = buttonState.lastEventTime - buttonState.firstHoldTime;

  // currently in long press status
  buttonState.isLongPressed = (buttonState.isPressed and buttonState.pressDuration > HOLD_BUTTON_MIN_MS);

  // remove button clicked if last call was too long ago (and an action is currently handled)
  if (buttonState.wasTriggered and ((buttonState.sinceLastCall > RELEASE_TIMING_MS) or
                                    (buttonState.isLongPressed and buttonState.sinceLastCall > RELEASE_TIMING_MS / 2)))
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
    buttonState.isPressed = false;
    buttonState.isLongPressed = false;
    buttonState.nbClicksCounted = 0;
    buttonState.firstHoldTime = buttonState.lastEventTime;
    // reset the action handling process
    buttonState.wasTriggered = false;
  }

  // set button high
  if (isButtonPressDetected)
  {
    // press detected, trigger
    buttonState.wasTriggered = true;

    // small delay since button press
    if (buttonState.sinceLastCall > RELEASE_BETWEEN_CLICKS)
    {
      if (!buttonState.isLongPressed)
      {
        buttonState.nbClicksCounted += 1;
        buttonState.firstHoldTime = buttonState.lastEventTime;
      }
    }

    buttonState.lastPressTime = buttonState.lastEventTime;
    buttonState.isPressed = true;
  }

  if (buttonState.isLongPressed)
  {
    // press detected, trigger
    buttonState.wasTriggered = true;

    clickHoldSerieCallback(buttonState.nbClicksCounted, buttonState.pressDuration);
  }
}

void handle_events(const std::function<void(uint8_t)>& clickSerieCallback,
                   const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback)
{
  // keep reading the button value until unpressed
  read_while_pressed();

  // check the button pressed status
  treat_button_pressed(wasButtonPressedDetected, clickSerieCallback, clickHoldSerieCallback);
}

} // namespace button

#endif