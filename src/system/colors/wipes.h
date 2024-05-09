#ifndef ANIMATIONS_WIPES_H
#define ANIMATIONS_WIPES_H

#include "../utils/strip.h"
#include "colors.h"

namespace animations {

/**
 * \brief Pass a color dot around the display, with a color, during a certain
 * duration. From top to bottom
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] fadeOut The animation fade speed (0: no fade)
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool dot_wipe_down(const Color& color, const uint32_t duration,
                   const uint8_t fadeOut, const bool restart, LedStrip& strip,
                   const float cutOff = 1);

/**
 * \brief Pass a color dot around the display, with a color, during a certain
 * duration. From bottom to top
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] fadeOut The animation fade speed (0: no fade)
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool dot_wipe_up(const Color& color, const uint32_t duration,
                 const uint8_t fadeOut, const bool restart, LedStrip& strip,
                 const float cutOff = 1);

/**
 * \brief Progressivly fill the display with a color, during a certain duration.
 * From top to bottom \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool color_wipe_down(const Color& color, const uint32_t duration,
                     const bool restart, LedStrip& strip,
                     const float cutOff = 1);

/**
 * \brief Progressivly fill the display with a color, during a certain duration.
 * From bottom to top \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool color_wipe_up(const Color& color, const uint32_t duration,
                   const bool restart, LedStrip& strip, const float cutOff = 1);

/**
 * \brief Progressivly fill the display with a color, during a certain duration.
 * From left to right \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool color_vertical_wipe_right(const Color& color, const uint32_t duration,
                               const bool restart, LedStrip& strip);

};  // namespace animations

#endif