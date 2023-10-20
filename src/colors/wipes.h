#ifndef ANIMATIONS_WIPES_H
#define ANIMATIONS_WIPES_H

#include "colors.h"

#include "../utils/strip.h"

namespace animations
{

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From top to bottom
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool dotWipeDown(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff=1);

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From bottom to top
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool dotWipeUp(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff=1);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From top to bottom
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff=1);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From bottom to top
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff=1);

};

#endif