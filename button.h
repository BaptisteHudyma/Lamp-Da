#ifndef BUTTON_H
#define BUTTON_H

#include <Adafruit_NeoPixel.h>
#include <functional>

#define BUTTON_PIN D2

#define HOLD_BUTTON_MIN_MS 500  // press and hold delay (ms)

/**
 * \brief handle the button clicked events
 * \param[in] clickSerieCallback A callback for a seri of clicks. Parameter is the number of consequtive clicks detected
 * \param[in] clickHoldSerieCallback A callback for a seri of clicks followed by a long hold. Parameter is the number of consequtive clicks detected and the time of the old event (in milliseconds). Called at until the button is released
 */
void handle_button_events(std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback);

#endif
