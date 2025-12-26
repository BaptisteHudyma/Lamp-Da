#ifndef ANIMATIONS_ANIMATIONS_H
#define ANIMATIONS_ANIMATIONS_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/colors/colors.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

namespace animations {

/**
 * \brief Fill the display with a color, with an optional cutoff value
 * \param[in] color class that returns a color to display
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of
 */
void fill(const Color& color, LedStrip& strip, const float cutOff = 1);

/**
 * \brief Do a wipe down followed by a wipe up animation
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] fadeOut The animation fade speed (0: no fade)
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool dot_ping_pong(const Color& color,
                   const uint32_t duration,
                   const uint8_t fadeOut,
                   const bool restart,
                   LedStrip& strip,
                   const float cutOff = 1);

/**
 * \brief Fill the display from both side simultaneously
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool double_side_fill(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip);

/**
 * \brief Do police light animation
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool police(const uint32_t duration, const bool restart, LedStrip& strip);

void show_text(const Color& color, const std::string& text, LedStrip& strip);

}; // namespace animations

#endif

#endif
