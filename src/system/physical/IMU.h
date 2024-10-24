#ifndef IMU_H
#define IMU_H

#include "../colors/animations.h"

/// Contains the handling of the gyroscope and accelerometer, and some
/// associated animations
namespace imu {

// start the imu readings
extern void enable();
// close the imu readings
extern void disable();

// disable imu if last use is old
extern void disable_after_non_use();

void gravity_fluid(const uint8_t fade, const Color& color, LedStrip& strip,
                   const bool isReset);

}  // namespace imu

#endif