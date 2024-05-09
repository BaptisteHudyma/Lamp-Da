#ifndef IMU_H
#define IMU_H

#include "../colors/animations.h"

/// Contains the handling of the gyroscope and accelerometer, and some
/// associated animations
namespace imu {

// start the imu readings
void enable_imu();
// close the imu readings
void disable_imu();

void gravity_fluid(const uint8_t fade, const Color& color, LedStrip& strip,
                   const bool isReset);

}  // namespace imu

#endif