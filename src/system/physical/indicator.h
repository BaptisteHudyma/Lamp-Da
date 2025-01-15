#ifndef PHYSICAL_INDICATOR_H
#define PHYSICAL_INDICATOR_H

#include "src/system/utils/colorspace.h"

namespace indicator {
extern void init();

extern void set_color(utils::ColorSpace::RGB c);

/**
 * \brief Breeze animation
 * \param[in] periodOn in mS, on time
 * \param[in] periodOff in mS, off time
 * \param[in] color The color of the animation
 * \return true when the animation finished. It loops automatically if called again
 */
extern bool breeze(const uint32_t periodOn, const uint32_t periodOff, const utils::ColorSpace::RGB& color);

/**
 * \brief Blink animation
 * \param[in] periodOn in mS, on time
 * \param[in] periodOff in mS, off time
 * \param[in] color The color of the animation
 * \return true when the animation finished. It loops automatically if called again
 */
extern bool blink(const uint32_t offFreq, const uint32_t onFreq, utils::ColorSpace::RGB color);

} // namespace indicator

#endif