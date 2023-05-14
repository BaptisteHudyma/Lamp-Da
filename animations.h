#ifndef ANIMATIONS_ANIMATIONS_H
#define ANIMATIONS_ANIMATIONS_H

#include <Adafruit_NeoPixel.h>
#include "colors.h"

namespace animations
{


/**
 * \brief Fill the display with a color, with an optional cutoff value
 * \param[in] color class that returns a color to display
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 */
void fill(const Color& color, Adafruit_NeoPixel& strip, const float cutOff=1);


/**
 * \brief Do a wipe down followed by a wipe up animation
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool dotPingPong(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff=1);

/**
 * \brief Do color pulse
 * \param[in] color class that returns a color to display
 * \param[in] durationPulseUp The duration of the color fill up animation, in milliseconds
 * \param[in] durationPulseDown The duration of the pong animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this color will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool colorPulse(const Color& color, const uint32_t durationPulseUp, const uint32_t durationPulseDown, const bool restart, Adafruit_NeoPixel& strip, const float cutOff=1);

/**
 * \brief Fill the display from both side simultaneously
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool doubleSideFillUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip);

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

/**
 * \brief Do a fade in of a color
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to contro
 * \param[in] firstCutOff between 0 and 1, how much this color will fill the display before suddently cutting of
 * \param[in] secondCutOff between 0 and 1, how much this color will fill the display before suddently cutting of
 * \return True if the animation is finished
 */
bool fadeIn(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float firstCutOff=0.0, const float secondCutOff=1.0);

};

#endif