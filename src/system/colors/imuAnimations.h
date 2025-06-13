#ifndef IMU_ANIMATIONS_H
#define IMU_ANIMATIONS_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/colors/colors.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

namespace animations {

void liquid(const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart);

void rain(const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart);

} // namespace animations

#endif

#endif