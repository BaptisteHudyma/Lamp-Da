#ifndef PHYSICAL_BUTTON_CPP
#define PHYSICAL_BUTTON_CPP

#include "button.h"

#include "src/system/hal/mutex.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"

#include "src/system/logic/inputs.h"
#include "src/system/logic/statistics_handler.h"

#include <atomic>

namespace lampda {
namespace component {
namespace button {

bool canRunButtonTask = false;

// The button pullup pin (one button pin to GND, the other to this pin)
hal::gpio::DigitalPin::GPIO _buttonPin = hal::gpio::DigitalPin::GPIO::gpio3;
hal::gpio::DigitalPin _buttonGpio(_buttonPin);

hal::gpio::DigitalPin::GPIO get_button_pin() { return _buttonPin; }
int get_button_pin_RAW() { return _buttonGpio.pin(); }

/// Button pressed states. Shared by the button interrupt, the button thread and the readers (main loop): only access
/// it under _buttonStateMutex
static ButtonStateTy buttonState = ButtonStateTy();

/// Guards buttonState. The interrupt never blocks on it, see button_state_interrupt(). Not recursive
static hal::mutex::hal_mutex_t _buttonStateMutex;
/// Initialized at program start: the state is read before the button init
[[maybe_unused]] static const bool _isButtonStateMutexInit = (hal::mutex::hal_mutex_init(&_buttonStateMutex), true);

/// Hold _buttonStateMutex for the scope
struct ButtonStateLock
{
  ButtonStateLock() { hal::mutex::hal_mutex_lock(&_buttonStateMutex); }
  ~ButtonStateLock() { hal::mutex::hal_mutex_unlock(&_buttonStateMutex); }
  ButtonStateLock(const ButtonStateLock&) = delete;
  ButtonStateLock& operator=(const ButtonStateLock&) = delete;
};

ButtonStateTy get_button_state()
{
  ButtonStateLock lock;
  return buttonState;
}

bool is_button_pressed()
{
  // this is a pullup, so high means no button press
  return not _buttonGpio.is_high();
}

/// Set by the interrupt on each pin change
static std::atomic<bool> wasButtonPressedDetected {false};

/// A press seen by the interrupt while buttonState was locked: registered later by the button thread
static std::atomic<bool> isPressPending {false};
/// Time of the pending press, valid when isPressPending is set
static std::atomic<uint32_t> pendingPressTime_ms {0};

/// Register a button press at \p pressTime. Call with _buttonStateMutex held. Return true if a new click was counted
static bool register_press_locked(const uint32_t pressTime)
{
  bool isNewClick = false;
  // small delay since button press, register a click
  if (!buttonState.isLongPressed and (pressTime - buttonState.lastPressTime) > RELEASE_BETWEEN_CLICKS)
  {
    buttonState.nbClicksCounted += 1;
    buttonState.firstHoldTime = pressTime;
    isNewClick = true;
  }
  // update trigger flags
  buttonState.lastPressTime = pressTime;
  buttonState.wasTriggered = true;
  buttonState.isPressed = true;
  return isNewClick;
}

/// Register the press deferred by the interrupt, if any. Call with _buttonStateMutex held.
/// Return true if a new click was counted
static bool register_pending_press_locked()
{
  if (not isPressPending.exchange(false))
    return false;

  uint32_t pressTime = pendingPressTime_ms.load();
  // a press older than the latest registered one belongs to it (elapsed times are safe across a time_ms wrap)
  const uint32_t currentTime = hal::time_ms();
  if ((currentTime - pressTime) > (currentTime - buttonState.lastPressTime))
    pressTime = buttonState.lastPressTime;
  return register_press_locked(pressTime);
}

void button_state_interrupt()
{
  // Runs in interrupt context on the hardware: it must never block on the state lock.
  // If the state is in use, the press is deferred to the button thread, see register_pending_press_locked()
  const bool isbuttonStillpressed = is_button_pressed();
  if (isbuttonStillpressed)
  {
    const uint32_t currentTime = hal::time_ms();
    if (hal::mutex::hal_mutex_trylock(&_buttonStateMutex) == 0)
    {
      const bool isNewClick = register_press_locked(currentTime);
      hal::mutex::hal_mutex_unlock(&_buttonStateMutex);

      // stat update
      if (isNewClick)
        logic::statistics::signal_button_press();
    }
    else
    {
      // time first: it is valid once the flag is seen
      pendingPressTime_ms.store(currentTime);
      isPressPending.store(true);
    }
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
  // Decided under the state lock, sent after it: the event queue has its own lock, and the prints are slow
  bool isNewPendingClick = false;
  bool isEndOfHold = false;
  bool isEndOfClicks = false;
  bool isHold = false;
  bool shouldShortcutInterrupt = false;
  uint8_t nbClicks = 0;
  uint32_t pressDuration = 0;
  {
    ButtonStateLock lock;
    isNewPendingClick = register_pending_press_locked();

    const bool isButtonPressDetected = wasButtonPressedDetected;
    const uint32_t currentTime = hal::time_ms();
    const uint32_t sinceLastCall = currentTime - buttonState.lastPressTime;
    pressDuration = currentTime - buttonState.firstHoldTime;

    // currently in long press status
    buttonState.isLongPressed = (buttonState.isPressed and pressDuration > HOLD_BUTTON_MIN_MS);

    // remove button clicked if last call was too long ago (and an action is currently handled)
    if (buttonState.wasTriggered and ((not buttonState.isLongPressed and sinceLastCall > RELEASE_TIMING_CLICKS_MS) or
                                      (buttonState.isLongPressed and sinceLastCall > RELEASE_TIMING_HOLDS_MS)))
    {
      // end of button press, trigger callback (press-hold action, or press action)
      nbClicks = buttonState.nbClicksCounted;
      isEndOfHold = buttonState.isLongPressed;
      isEndOfClicks = not buttonState.isLongPressed;

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
      nbClicks = buttonState.nbClicksCounted;
      isHold = true;
    }

    // safety : an interrupt may have been missed, and the button is locked in a logic pressed state
    shouldShortcutInterrupt =
            isButtonPressDetected && not is_button_pressed() && pressDuration > RELEASE_BETWEEN_CLICKS;
  }

  if (isNewPendingClick)
    logic::statistics::signal_button_press();

  if (isEndOfHold)
  {
    const bool isSuccess = logic::inputs::add_button_press_event(nbClicks, pressDuration, true);
    if (not isSuccess)
    {
      bsp::lampda_print("Button: Could not register end of hold event");
    }
    else
      bsp::lampda_print(
              "Button: Registered %d clicks and %lu ms press", nbClicks, static_cast<unsigned long>(pressDuration));
  }
  else if (isEndOfClicks)
  {
    const bool isSuccess = logic::inputs::add_button_click_event(nbClicks);
    if (not isSuccess)
    {
      bsp::lampda_print("Button: Could not register end of click event, droping less important events");
    }
    else
      bsp::lampda_print("Button: Registered %d clicks", nbClicks);
  }

  if (isHold)
  {
    // if not successful: ok, just a ramp event miss can happen without a problem
    logic::inputs::add_button_press_event(nbClicks, pressDuration, false);
  }

  if (shouldShortcutInterrupt)
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

  {
    // if button if already started, reset it
    ButtonStateLock lock;
    buttonState.reset();
    isPressPending = false;
  }

  // attach the button interrupt
  init_button_gpio();

  // prevent multiple clicks on start
  bool isStartClickSimulated = false;
  {
    ButtonStateLock lock;
    if (isSystemStartedFromButton and buttonState.nbClicksCounted == 0)
    {
      // simulate a click
      wasButtonPressedDetected = true;
      buttonState.nbClicksCounted = 1;
      buttonState.lastPressTime = hal::time_ms();
      buttonState.firstHoldTime = hal::time_ms();
      buttonState.wasTriggered = true;
      buttonState.isPressed = true;
      isStartClickSimulated = true;
    }
  }
  if (isStartClickSimulated)
    logic::statistics::signal_button_press();

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
