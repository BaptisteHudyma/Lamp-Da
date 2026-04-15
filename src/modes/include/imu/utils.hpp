/*! \file utils.hpp
    \brief User mode IMU utilities.
*/

#ifndef MODES_INCLUDE_IMU_UTILS_HPP
#define MODES_INCLUDE_IMU_UTILS_HPP

#include "src/system/physical/imu.h"
#include "src/system/utils/vector_math.h"

#include "src/modes/include/particle_system/particle_system.hpp"

/// @file utils.hpp

/// User modes imu utilities
namespace modes::imu {

template<int temp = 0> struct ImuEventTy
{
  /// last reading of the IMU
  lampda::physical::imu::Reading lastReading;

  /// Reset the IMU events.
  /// Should be called before any use
  void reset(auto& ctx)
  {
    // reset filter
    lampda::physical::imu::get_filtered_reading(true);

    // reset particles
    particuleSystem.reset();
    particuleSystem.set_max_particle_count(512);
  }

  /// Call this once every tick inside the mode loop callback
  void update(auto& ctx)
  {
    //
    lastReading = lampda::physical::imu::get_filtered_reading(false);
  }

  /// ALL IMU ANIMATIONS SHARE THIS PARTICLE SYSTEM.
  /// Spawn another if multiple systems should run in parralel
  modes::ParticleSystem particuleSystem;

private:
};

} // namespace modes::imu

#endif
