#ifndef IMU_ANIMATIONS_H
#define IMU_ANIMATIONS_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/colors/colors.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

namespace animations {

/**
 * \brief Simulate a dense fluid
 * \param[in] persistance persistance vision for each particle. 0 no peristance (dots), 255 full persistance (never
 * erased)
 */
void liquid(const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart);

/**
 * \brief Simulate a dense fluid
 * \param[in] rainDensity 0: very low rain, few drops per second, 255 heavy fast rain
 * \param[in] persistance persistance vision for each particle. 0 no peristance (dots), 255 full persistance (never
 * erased)
 */
void rain(
        const uint8_t rainDensity, const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart);

} // namespace animations

#endif

#endif