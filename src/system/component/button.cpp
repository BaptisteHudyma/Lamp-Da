#ifndef PHYSICAL_BUTTON_CPP
#define PHYSICAL_BUTTON_CPP

#include "button.h"

#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"

#include "src/system/logic/inputs.h"
#include "src/system/logic/statistics_handler.h"

namespace lampda {
namespace component {
namespace button {

bool canRunButtonTask = false;

// The button pullup pin (one button pin to GND, the other to this pin)
hal::gpio::DigitalPin::GPIO _buttonPin = hal::gpio::DigitalPin::GPIO::gpio3;
hal::gpio::DigitalPin _buttonGpio(_buttonPin);

hal::gpio::DigitalPin::GPIO get_button_pin() { return _buttonPin; }
int get_button_pin_RAW() { return _buttonGpio.pin(); }

// button pressed states
static ButtonStateTy buttonState = ButtonStateTy();
ButtonStateTy get_button_state() { return buttonState; }

bool is_button_pressed()
{
  // this is a pullup, so high means no button press
  return not _buttonGpio.is_high();
}

static volatile bool wasButtonPressedDetected = false;
void button_state_interrupt()
{
  // There is a bit too much operations for an interrupt, but it's supposed to be rare
  // And it's ok for a real time kernel
  const bool isbuttonStillpressed = is_button_pressed();
  if (isbuttonStillpressed)
  {
    const uint32_t currentTime = hal::time_ms();
    // small delay since button press, register a click
    if (!buttonState.isLongPressed and (currentTime - buttonState.lastPressTime) > RELEASE_BETWEEN_CLICKS)
    {
      buttonState.nbClicksCounted += 1;
      buttonState.firstHoldTime = currentTime;

      // stat update
      logic::statistics::signal_button_press();
    }
    // update trigger flags
    buttonState.lastPressTime = currentTime;
    buttonState.wasTriggered = true;
    buttonState.isPressed = true;
  }

  // update flag
  wasButtonPressedDetected = isbuttonStillpressed;
}

/// Init the button gpio to the set _buttonPin
void init_button_gpio()
{
  _buttonGpio.detach_callbacks();
  _buttonGpio.set(_buttonPin);
  _buttonGpio.set_pin_mode(hal::gpio::DigitalPin::Mode::kInputPullUpSense);
  _buttonGpio.attach_callback(button_state_interrupt, hal::gpio::DigitalPin::Interrupt::kChange);
}

void set_button_pin(const hal::gpio::DigitalPin::GPIO buttonPin)
{
  if (_buttonPin != buttonPin)
  {
    _buttonPin = buttonPin;
    // reinit on the fly
    init_button_gpio();
  }
}

void handle_events()
{
  const bool isButtonPressDetected = wasButtonPressedDetected;
  const uint32_t currentTime = hal::time_ms();
  const uint32_t sinceLastCall = currentTime - buttonState.lastPressTime;
  const uint32_t pressDuration = currentTime - buttonState.firstHoldTime;

  // currently in long press status
  buttonState.isLongPressed = (buttonState.isPressed and pressDuration > HOLD_BUTTON_MIN_MS);

  // remove button clicked if last call was too long ago (and an action is currently handled)
  if (buttonState.wasTriggered and ((not buttonState.isLongPressed and sinceLastCall > RELEASE_TIMING_CLICKS_MS) or
                                    (buttonState.isLongPressed and sinceLastCall > RELEASE_TIMING_HOLDS_MS)))
  {
    // end of button press, trigger callback (press-hold action, or press action)
    if (buttonState.isLongPressed)
    {
      const bool isSuccess = logic::inputs::add_button_press_event(buttonState.nbClicksCounted, pressDuration, true);
      if (not isSuccess)
      {
        bsp::lampda_print("Button: Could not register end of hold event");
      }
      else
        bsp::lampda_print("Button: Registered %d clicks and %d ms press", buttonState.nbClicksCounted, pressDuration);
    }
    else
    {
      const bool isSuccess = logic::inputs::add_button_click_event(buttonState.nbClicksCounted);
      if (not isSuccess)
      {
        bsp::lampda_print("Button: Could not register end of click event, droping less important events");
      }
      else
        bsp::lampda_print("Button: Registered %d clicks", buttonState.nbClicksCounted);
    }

    // reset
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

    const bool isSuccess = logic::inputs::add_button_press_event(buttonState.nbClicksCounted, pressDuration, false);
    // if (not isSuccess) : ok: just a ramp event miss can happen without a problem
  }

  // safety : an interrupt may have been missed, and the button is locked in a logic pressed state
  if (isButtonPressDetected && not is_button_pressed() && pressDuration > RELEASE_BETWEEN_CLICKS)
  {
    button_state_interrupt();
    bsp::lampda_print("Button interrupt shortcut due to state lock detected");
  }
}

void button_thread()
{
  if (canRunButtonTask)
  {
    handle_events();
  }
  hal::delay_ms(thread_throttle_time_ms);
}

void init(const bool isSystemStartedFromButton)
{
  static_assert(RELEASE_BETWEEN_CLICKS < RELEASE_TIMING_CLICKS_MS,
                "release debounce should always be less than click release timing");
  static_assert((RELEASE_BETWEEN_CLICKS + RELEASE_TIMING_CLICKS_MS) < HOLD_BUTTON_MIN_MS,
                "button click release timing should always be less then the button hold timing");
  static_assert(RELEASE_BETWEEN_CLICKS < RELEASE_TIMING_HOLDS_MS,
                "release debounce should always be less than hold release timing");
  static_assert((RELEASE_BETWEEN_CLICKS + RELEASE_TIMING_HOLDS_MS) < HOLD_BUTTON_MIN_MS,
                "button hold release timing should always be less then the button hold timing");
  static_assert(thread_throttle_time_ms < RELEASE_BETWEEN_CLICKS,
                "thread throttle should always be less than the release timing");

  // if button if already started, reset it
  buttonState.reset();

  // attach the button interrupt
  init_button_gpio();

  // prevent multiple clicks on start
  if (isSystemStartedFromButton and buttonState.nbClicksCounted == 0)
  {
    // simulate a click
    wasButtonPressedDetected = true;
    buttonState.nbClicksCounted = 1;
    buttonState.lastPressTime = hal::time_ms();
    buttonState.firstHoldTime = hal::time_ms();
    buttonState.wasTriggered = true;
    buttonState.isPressed = true;

    logic::statistics::signal_button_press();
  }

  canRunButtonTask = true;

  static constexpr uint16_t buttonThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t buttonThreadBuffer[buttonThreadBufferSize];
  bsp::threads::start_thread(
          button_thread, bsp::threads::button_taskName, 2, buttonThreadBufferSize, buttonThreadBuffer);
}

void shutdown() { canRunButtonTask = false; }

} // namespace button
} // namespace component
} // namespace lampda

#endif
