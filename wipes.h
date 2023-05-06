#ifndef ANIMATIONS_WIPES_H
#define ANIMATIONS_WIPES_H

#include <Adafruit_NeoPixel.h>

namespace animations
{

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From top to bottom
 * \param[in] color The color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotWipeDown(const uint32_t color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Pass a color dot around the display, with a color, during a certain duration. From bottom to top
 * \param[in] color The color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotWipeUp(const uint32_t color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Pass a rainbow dot around the display, during a certain duration. From top to bottom
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotWipeDownRainbow(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From top to bottom
 * \param[in] color The color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool colorWipeDown(const uint32_t color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Progressivly fill the display with a color, during a certain duration. From bottom to top
 * \param[in] color The color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool colorWipeUp(const uint32_t color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

};

#endif