#ifndef IMU_WRAPPER_H
#define IMU_WRAPPER_H

namespace imu {

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

class Wrapper
{
public:
  bool init();
  bool shutdown();

  // get the accelerometer/gyroscope measurment
  Reading get_reading();

  enum InterruptType
  {
    None,        // no interrupt
    Fall,        // raised during a free fall event
    BigMotion,   // raised with a >6g acceleration
    Step,        // raised on a step event
    AngleChange, // raised on portrait to landscape (or inverse) rotation
  };

  // enable an event detection
  bool enable_detection(const InterruptType interr);
  // disable event detection. WILL ALSO DISABLE ASSOCIATED INTERRUPTS
  void disable_detection(const InterruptType interr);

  bool enable_interrupt1(const InterruptType interr);

  // return true if the interrupt is raised, do not depend on physical interrupt pins
  bool is_event_detected(const InterruptType interr);

protected:
  bool enable_free_fall_detection();
  bool disable_free_fall_detection();

  bool enable_big_motion_detection();
  bool disable_big_motion_detection();

private:
};

} // namespace imu

#endif