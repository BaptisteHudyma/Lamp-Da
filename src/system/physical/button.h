/*! \file button.h
    \brief Interface for the physical components of the push button.
*/

#ifndef BUTTON_H
#define BUTTON_H

#include <cstdint>
#include <functional>
#include "src/system/platform/gpio.h"

namespace lampda {
namespace physical {
/// Handle the button inputs and multiple click detection.
namespace button {

static constexpr uint32_t thread_throttle_time_ms = 10;   ///< thread run frequency timing
static constexpr uint32_t RELEASE_BETWEEN_CLICKS = 50;    ///< minimum release timing (debounce)
static constexpr uint32_t RELEASE_TIMING_CLICKS_MS = 250; ///< time to release the button after clicks stops
static constexpr uint32_t RELEASE_TIMING_HOLDS_MS = 125;  ///< time to release the button after hold stops
static constexpr uint32_t HOLD_BUTTON_MIN_MS = 500;       ///< press and hold delay (ms)

/**
 * \brief Start the button handler
 * \param[in] isSystemStartedFromButton Signal to the button that the system was started by a button click
 */
void init(const bool isSystemStartedFromButton);

/**
 * \brief Store the button status and characteristics
 */
struct ButtonStateTy
{
  /// Is the button pressed?
  bool isPressed = false;
  /// Is button in long press?
  bool isLongPressed = false;
  /// Timestamp (millis) of last press
  uint32_t lastPressTime = 0;
  /// Timestamp (millis) of first press (hold)
  uint32_t firstHoldTime = 0;
  /// Nb of counted clicks
  uint8_t nbClicksCounted = 0;
  /// Was a button action detected
  bool wasTriggered = false;

  /// Reset this object
  void reset()
  {
    isPressed = false;
    isLongPressed = false;
    lastPressTime = 0;
    firstHoldTime = 0;
    nbClicksCounted = 0;
  }
};

/**
 * \brief Get a copy of the raw internal button state
 */
extern ButtonStateTy get_button_state();

/**
 * \brief Return the pin used for the button
 */
extern platform::gpio::DigitalPin::GPIO get_button_pin();

/**
 * \brief Only on system start, set the pin where the button is wired
 */
extern void set_button_pin(const platform::gpio::DigitalPin::GPIO buttonPin);

/**
 * \brief get the button pin index in system. USE WITH CAUTION
 */
extern int get_button_pin_RAW();

} // namespace button
} // namespace physical
} // namespace lampda

#endif
