#ifndef IMU_ANIMATIONS_H
#define IMU_ANIMATIONS_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/colors/colors.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

#include "src/system/physical/imu.h"

namespace animations {

void liquid(LedStrip& strip);

}

#endif

#endif