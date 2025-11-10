#ifndef IMU_WRAPPER_H
#define IMU_WRAPPER_H

#include <cstdint>

#include "src/system/utils/vector_math.h"

namespace imu {

struct Reading
{
  // accelerometer in G
  vec3d accel;
  // gyroscopic speed in degree per second
  vec3d gyro;
};

class Wrapper
{
public:
  bool init() const;
  bool shutdown() const;

  // get the accelerometer/gyroscope measurment
  Reading get_reading() const;

  enum class InterruptType
  {
    FreeFall,    // raised during a free fall event
    BigMotion,   // raised with a >6g acceleration
    Step,        // raised on a step event
    AngleChange, // raised on portrait to landscape (or inverse) rotation
  };

  // free fall events
  bool enable_free_fall_detection() const;
  bool disable_free_fall_detection() const;

  // big motion
  bool enable_big_motion_detection() const;
  bool disable_big_motion_detection() const;

  // step motion
  bool enable_step_detection() const;
  bool disable_step_detection() const;

  // step motion
  bool enable_tilt_detection() const;
  bool disable_tilt_detection() const;

  // disable event detection. WILL ALSO DISABLE ASSOCIATED INTERRUPTS
  void disable_detection(const InterruptType interr) const;

  bool enable_interrupt1(const InterruptType interr) const;
  void disable_interrupt1() const;

  bool enable_interrupt2(const InterruptType interr) const;
  void disable_interrupt2() const;

  uint16_t get_step_count() const;

  // return true if the interrupt is raised, do not depend on physical interrupt pins
  bool is_event_detected(const InterruptType interr) const;
};

} // namespace imu

#endif
