#ifndef IMU_H
#define IMU_H

/// Contains the handling of the gyroscope and accelerometer

// IMU is auto activated when used
namespace imu {

// close the imu readings
extern void disable();

// disable imu if last use is old
extern void disable_after_non_use();

enum EventType
{
  FreeFall,  // raised during a free fall event
  BigMotion, // raised during a big acceleration
};

// enable the interrupt 1 with an event type
extern bool enable_interrupt_1(EventType eventType);
extern void disable_interrupt_1();

// read and reset the interrupt bit
bool is_interrupt1_enabled();

struct vec3d
{
  float x;
  float y;
  float z;
};

struct Accelerometer : public vec3d
{
};
struct Gyroscope : public vec3d
{
};

struct Reading
{
  Accelerometer accel;
  Gyroscope gyro;
};

//
Reading get_filtered_reading(const bool resetFilter);

} // namespace imu

#endif