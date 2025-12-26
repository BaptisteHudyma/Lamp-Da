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
 * \brief Do police light animation
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool police(const uint32_t duration, const bool restart, LedStrip& strip);

}; // namespace animations

#endif

#endif
