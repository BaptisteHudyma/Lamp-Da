#ifndef BUTTON_H
#define BUTTON_H

#include <cstdint>
#include <functional>

#include "src/system/utils/colorspace.h"

namespace button {

#define HOLD_BUTTON_MIN_MS 500  // press and hold delay (ms)

void init();

/**
 * \brief handle the button clicked events
 * \param[in] clickSerieCallback A callback for a seri of clicks. Parameter is
 * the number of consequtive clicks detected
 * \param[in] clickHoldSerieCallback A callback for a seri of clicks followed
 * by a long hold. Parameter is the number of consequtive clicks detected and
 * the time of the old event (in milliseconds). Called at until the button is
 * released
 */
void handle_events(
    const std::function<void(uint8_t)>& clickSerieCallback,
    const std::function<void(uint8_t, uint32_t)>& clickHoldSerieCallback);

/**
 * Display a color on the button
 */
void set_color(utils::ColorSpace::RGB color);
void blink(const uint32_t offFreq, const uint32_t onFreq,
           utils::ColorSpace::RGB color);

/**
 * \brief Make the breeze animation on the button
 */
void breeze(const uint32_t periodOn, const uint32_t periodOff,
            const utils::ColorSpace::RGB& color);

// Button state
struct ButtonStateTy {
  bool isPressed = false; // is the button pressed?
  bool isLongPressed = false; // is button in long press?
  uint32_t lastPressTime = 0; // timestamp (millis) of last press
  uint32_t firstHoldTime = 0; // timestamp (millis) of first press (hold)
  uint8_t nbClicksCounted = 0; // nb of counted clicks

  bool wasTriggered = false; // was button triggered last update?
  uint32_t pressDuration = 0; // estimated (millis) press duration
  uint32_t sinceLastCall = 0; // time (unprocessed) since last call
  uint32_t lastEventTime = 0; // last (unprocessed) time of call
  bool lastRawButton = false; // last (unprocessed) boolean value read
};

/**
 * \brief Get a copy of the raw internal button state
 */
extern ButtonStateTy get_button_state();


}  // namespace button

#endif
