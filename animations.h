#ifndef ANIMATIONS_ANIMATIONS_H
#define ANIMATIONS_ANIMATIONS_H

#include <Adafruit_NeoPixel.h>

namespace animations
{

/**
 * \brief Do a wipe down followed by a wipe up animation
 * \param[in] color The color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool dotPingPong(const uint32_t color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Do police light animation
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool police(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

/**
 * \brief Do a fade out of the colors currently displayed
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool fadeOut(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);


void rainbowFade2White(int wait, int rainbowLoops, Adafruit_NeoPixel& strip);

};

#endif