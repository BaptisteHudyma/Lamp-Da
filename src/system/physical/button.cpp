#include "button.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#define RELEASE_TIMING_MS      200
#define RELEASE_BETWEEN_CLICKS 50

namespace button {

static ButtonStateTy buttonState = ButtonStateTy();
ButtonStateTy get_button_state() { return buttonState; }

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt() { wasButtonPressedDetected = true; }

void init()
{
  pinMode(BUTTON_RED, OUTPUT);
  pinMode(BUTTON_GREEN, OUTPUT);
  pinMode(BUTTON_BLUE, OUTPUT);

  set_color(utils::ColorSpace::BLACK);

  // attach the button interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP_SENSE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_state_interrupt, CHANGE);
}

static volatile bool buttonPressListener = false;
void read_while_pressed()
{
  if (wasButtonPressedDetected and digitalRead(BUTTON_PIN) == HIGH)
  {
    wasButtonPressedDetected = false;
  }
}

void treat_button_pressed(const bool isButtonPressDetected,
                          const std::function<void(uint8_t)>& clickSerieCallback,
                          const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback)
{
  buttonState.lastEventTime = millis();
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

void set_color(utils::ColorSpace::RGB color)
{
  static constexpr float redColorCorrection = 1.0;
  static constexpr float greenColorCorrection =
          1.0 / 7.5; // the green of this button is way way higher than the other colors
  static constexpr float blueColorCorrection = 1.0;

  const COLOR& col = color.get_rgb();
  analogWrite(BUTTON_RED, col.red * redColorCorrection);
  analogWrite(BUTTON_GREEN, col.green * greenColorCorrection);
  analogWrite(BUTTON_BLUE, col.blue * blueColorCorrection);
}

void blink(const uint32_t offFreq, const uint32_t onFreq, utils::ColorSpace::RGB color)
{
  static uint32_t lastCall = 0;
  static bool ledState = false;

  // led is off, and last call was 100ms before
  if (not ledState and millis() - lastCall > onFreq)
  {
    ledState = true;
    set_color(color);
    lastCall = millis();
  }

  // led is on, and last call was long ago
  if (ledState and millis() - lastCall > offFreq)
  {
    ledState = false;
    // set black
    set_color(utils::ColorSpace::RGB(0));
    lastCall = millis();
  }
}

void breeze(const uint32_t periodOn, const uint32_t periodOff, const utils::ColorSpace::RGB& color)
{
  const uint32_t time = millis();

  // store the start time of the animation
  static uint32_t startTime = time;

  // breeze on
  const uint32_t timeSinceStart = time - startTime;
  if (timeSinceStart < periodOn)
  {
    const float progression = utils::map(timeSinceStart, 0, periodOn, 0.0, 1.0);

    // rising edge
    if (progression <= 0.5)
    {
      // map from [0.0; 0.5] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0 * progression)));
    }
    // falling edge
    else
    {
      // map from ]0.5; 1.0] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0 * (1.0 - progression))));
    }
  }
  // breeze off
  else if (timeSinceStart < periodOn + periodOff)
  {
    set_color(utils::ColorSpace::BLACK);
  }
  else
  {
    // reset animation
    startTime = time;
    set_color(utils::ColorSpace::BLACK);
  }
}

} // namespace button
