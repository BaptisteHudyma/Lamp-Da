#ifndef BUTTON_H
#define BUTTON_H

#include <cstdint>
#include <functional>

namespace button {

#define RELEASE_BETWEEN_CLICKS 50  // minimum release timing (debounce)
#define RELEASE_TIMING_MS      250 // time to release the button after no inputs
#define HOLD_BUTTON_MIN_MS     500 // press and hold delay (ms)

void init(const bool isSystemStartedFromButton);

/**
 * \brief handle the button clicked events
 * \param[in] clickSerieCallback A callback for a seri of clicks. Parameter is
 * the number of consequtive clicks detected
 * \param[in] clickHoldSerieCallback A callback for a seri of clicks followed
 * by a long hold. Parameter is the number of consequtive clicks detected and
 * the time of the old event (in milliseconds). Called at until the button is
 * released
 */
void handle_events(const std::function<void(uint8_t)>& clickSerieCallback,
                   const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback);

/**
 * \brief Indicates that this click is the one triggered by the system start
 * It is set to false after the click chain stops
 */
bool is_system_start_click();

// Button state
struct ButtonStateTy
{
  bool isPressed = false;      // is the button pressed?
  bool isLongPressed = false;  // is button in long press?
  uint32_t lastPressTime = 0;  // timestamp (millis) of last press
  uint32_t firstHoldTime = 0;  // timestamp (millis) of first press (hold)
  uint8_t nbClicksCounted = 0; // nb of counted clicks

  bool wasTriggered = false; // was button action detected
};

/**
 * \brief Get a copy of the raw internal button state
 */
extern ButtonStateTy get_button_state();

} // namespace button

#endif
