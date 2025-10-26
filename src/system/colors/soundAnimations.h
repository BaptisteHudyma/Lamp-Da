#ifndef SOUND_ANIMATION_H
#define SOUND_ANIMATION_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/colors/colors.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

#include "src/system/physical/sound.h"

namespace animations {

void fft_display(const uint8_t speed, const uint8_t scale, const palette_t& palette, LedStrip& strip);

} // namespace animations

#endif

#endif
