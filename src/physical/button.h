#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include <cstdint>
#include "../utils/colorspace.h"

#define HOLD_BUTTON_MIN_MS 500  // press and hold delay (ms)

void button_state_interrupt();

/**
 * \brief handle the button clicked events
 * \param[in] clickSerieCallback A callback for a seri of clicks. Parameter is the number of consequtive clicks detected
 * \param[in] clickHoldSerieCallback A callback for a seri of clicks followed by a long hold. Parameter is the number of consequtive clicks detected and the time of the old event (in milliseconds). Called at until the button is released
 */
void handle_button_events(std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback);

/**
 * Display a color on the button
 */
void set_button_color(utils::ColorSpace::RGB color);

// return a number between 0 and 100
float get_battery_level(const bool resetRead);

#endif
