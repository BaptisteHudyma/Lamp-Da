#ifndef ANIMATIONS_WIPES_H
#define ANIMATIONS_WIPES_H

#include "utils.h"
#include <Adafruit_NeoPixel.h>

namespace animations
{

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From top to bottom
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From bottom to top
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From top to bottom
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From bottom to top
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

};

#endif