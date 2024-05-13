#ifndef IMU_H
#define IMU_H

/// Contains the handling of the gyroscope and accelerometer, and some
/// associated animations
namespace imu {

// start the imu readings
void enable();
// close the imu readings
void disable();

}  // namespace imu

#endif