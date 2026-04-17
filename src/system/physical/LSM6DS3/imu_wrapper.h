/*! \file imu_wrapper.h
    \brief Interface for the physical components of the inertial motion unit.
*/

#ifndef IMU_WRAPPER_H
#define IMU_WRAPPER_H

#include <cstdint>

#include "src/system/utils/vector_math.h"

namespace lampda {
namespace physical {
/// physical layer of the IMU.
namespace imu {

/// IMU reading
struct Reading
{
  /// accelerometer in G
  utils::vec3d accel;
  /// gyroscopic speed in degree per second
  utils::vec3d gyro;
};

/// IMU wrapper class
class Wrapper
{
public:
  /**
   * \brief Initialize the component.
   * \return True if the initialisation succeeded
   */
  bool init() const;
  /**
   * \brief Gracefully shutdown the component.
   * \return True if the shutdown succeeded
   */
  bool shutdown() const;

  /// get the latest accelerometer/gyroscope measurment
  Reading get_reading() const;

  /// Interrupt type for the event callbacks.
  enum class InterruptType
  {
    FreeFall,    ///< raised during a free fall event
    BigMotion,   ///< raised with a >6g acceleration
    Step,        ///< raised on a step event
    AngleChange, ///< raised on portrait to landscape (or inverse) rotation
  };

  /// Enable free fall events detection
  bool enable_free_fall_detection() const;
  /// Disable free fall events detection
  bool disable_free_fall_detection() const;

  /// Enable big motion events detection
  bool enable_big_motion_detection() const;
  /// Disable big motion events detection
  bool disable_big_motion_detection() const;

  /// Enable step motion event detection
  bool enable_step_detection() const;
  /// Disable step motion event detection
  bool disable_step_detection() const;

  /// Enable tilt detection
  bool enable_tilt_detection() const;
  /// Disable tilt detection
  bool disable_tilt_detection() const;

  /// disable event detection
  /// \warning WILL ALSO DISABLE THE ASSOCIATED INTERRUPTS
  void disable_detection(const InterruptType interr) const;

  /**
   * \brief Enable the interrupt 1 signal, with the given interrupt type.
   * \param[in] interr The desired interrupt type
   * \return true if the interrupt gets enabled
   */
  bool enable_interrupt1(const InterruptType interr) const;
  /// Disable the interrupt 1 events
  void disable_interrupt1() const;

  /**
   * \brief Enable the interrupt 2 signal, with the given interrupt type.
   * \param[in] interr The desired interrupt type
   * \return true if the interrupt gets enabled
   */
  bool enable_interrupt2(const InterruptType interr) const;
  /// Disable the interrupt 2 events
  void disable_interrupt2() const;

  /// Return the step count, only available with the interrupt 2.
  uint16_t get_step_count() const;

  /// return true if the interrupt is raised, do not depend on physical interrupt pins
  bool is_event_detected(const InterruptType interr) const;
};

} // namespace imu
} // namespace physical
} // namespace lampda

#endif
