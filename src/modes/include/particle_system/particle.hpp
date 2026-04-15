/*! \file particle.hpp
    \brief Define a single particle for a particle system.
*/

#ifndef PARTICLE_H
#define PARTICLE_H

#include "src/user/constants.h"

#include "src/system/utils/vector_math.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/ext/math8.h"
#include <cstdint>

namespace modes {

/**
 * \brief Define a particle in cylindrical space, and movement equations on the cylinder surface
 */
using LampTy = hardware::LampTy;

static constexpr float cylinderRadius_m = LampTy::lampBodyRadius_mm / 1000.0; ///< radius of the cylinder, in meters
static constexpr float angularSpeedGain = 50000; ///< this gain compensate the angular acceleration for better display
static constexpr float linearSpeedGain = 1.0;    ///< gain for the linear speeds
static constexpr float reboundCoeff = 0.1;       ///< low rebound [0 - 1] on walls
static constexpr float speedDampening = 0.92;    ///< low speed dampening [0-0.99] at every steps (viscosity)

static constexpr float maxAngularSpeed_radS = 4 * c_TWO_PI; ///< max angular speed in radians/s
static constexpr float maxVerticalSpeed_mmS = 50;           ///< max vertical speed in mm/S

/**
 * \brief Define a particle in 3D space. it has a position and speed
 */
struct Particle
{
  /// Default particle init
  Particle() : thetaSpeed_radS(0.0), zSpeed_mS(0.0), theta_rad(0.0), z_mm(0.0) {}

  /// Construct a Particle from a 3D cartesian position.
  Particle(const vec3d& positionCartesian) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(atan2(positionCartesian.y, positionCartesian.x)),
    z_mm(positionCartesian.z)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  /// Construct a particle from an angle and height.
  Particle(const float positionTheta_rad, const float positionZ_mm) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(positionTheta_rad),
    z_mm(positionZ_mm)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  /// Copy constructor for a particle
  Particle(const Particle& other) = default;

  /**
   * \brief Compute the speed increment in the constraint lamp body
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \return The speed in angle and heigh.
   */
  vec2d compute_speed_increment(const vec3d& accelerationCartesian_m, const float deltaTime_s) const
  {
    // speed vector on radial (derivative of cartesian to cylinder coordinates for theta)
    const vec3d e_theta(-sin_t(theta_rad) * cylinderRadius_m, cos_t(theta_rad) * cylinderRadius_m, 0);
    // speed vector on z (derivative of cartesian to cylinder coordinates for z)
    const vec3d e_z(0, 0, 1);
    // ignore the radius derivative, as we want to stay on the cylinder surface

    // compute the speed vector on cylinder surface
    const vec3d& tangantialVector = e_theta.multiply(accelerationCartesian_m.dot(e_theta));
    const vec3d& directVector = e_z.multiply(accelerationCartesian_m.dot(e_z));
    const vec3d& accelerationVector = tangantialVector.add(directVector);

    return vec2d(angularSpeedGain * accelerationVector.dot(e_theta) / static_cast<float>(cylinderRadius_m) *
                         deltaTime_s,
                 linearSpeedGain * accelerationVector.dot(e_z) * deltaTime_s);
  }

  /**
   * \brief apply a cartesian acceleration to this particulate
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \param[in] shouldContrain If true, will constrain the movement into the lamp body.
   */
  void apply_acceleration(const vec3d& accelerationCartesian_m,
                          const float deltaTime_s,
                          const bool shouldContrain = true)
  {
    const auto& speedIncrement = compute_speed_increment(accelerationCartesian_m, deltaTime_s);
    // start by dampening the speed
    dampen_speed(speedDampening);

    // update speed
    thetaSpeed_radS =
            lmpd_constrain<float>(thetaSpeed_radS + speedIncrement.x, -maxAngularSpeed_radS, maxAngularSpeed_radS);
    zSpeed_mS = lmpd_constrain<float>(zSpeed_mS + speedIncrement.y, -maxVerticalSpeed_mmS, maxVerticalSpeed_mmS);

    static constexpr float angularUnit = LampTy::maxWidthFloat / c_TWO_PI;
    static constexpr float verticalUnit = LampTy::ledStripWidth_mm * 1.5f;

    // update position (limit speed to pixel unit per dt)
    const float angularPositionIncrement =
            lmpd_constrain<float>(thetaSpeed_radS * deltaTime_s, -angularUnit, angularUnit);
    const float verticalPositionIncrement =
            lmpd_constrain<float>(zSpeed_mS * deltaTime_s, -verticalUnit, verticalUnit) * 1000.0;

    // update particle position
    z_mm += verticalPositionIncrement;
    theta_rad += angularPositionIncrement;

    // limit the derivation of the angle
    theta_rad = wrap_angle(theta_rad);

    // constrain to the lamp body
    if (shouldContrain)
      constraint_into_lamp_body();
    else
      _savedLampIndex = to_lamp_index_no_bounds();
  }

  /**
   * \brief Simulate this particle movement as a new particle.
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \param[in] shouldContrain If true, will constrain the movement into the lamp body.
   */
  Particle simulate_after_acceleration(const vec3d& accelerationCartesian_m,
                                       const float deltaTime_s,
                                       const bool shouldContrain = true) const
  {
    Particle simulated(*this);
    simulated.apply_acceleration(accelerationCartesian_m, deltaTime_s, shouldContrain);
    return simulated;
  }

  /**
   * \brief Contrain the particle movement to the cylinder, limiting the movement at the extremities.
   */
  void constraint_into_lamp_body()
  {
    _savedLampIndex = to_lamp_index_no_bounds();

    // coordinates go from 0 to -max
    if (z_mm > 0)
    {
      z_mm = 0;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
      _savedLampIndex = to_lamp_index_no_bounds();
    }

    if (z_mm < -LampTy::lampHeight_mm)
    {
      z_mm = -LampTy::lampHeight_mm;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
      _savedLampIndex = to_lamp_index_no_bounds();
    }

    // handle the real limits (led strip do not start and end at zero depth)
    if (not is_led_index_valid(_savedLampIndex))
    {
      // we are too high above the first led
      if (z_mm >= 0)
      {
        // go one unit lower
        z_mm = LampTy::ledStripWidth_mm * 1.5;
        zSpeed_mS = -zSpeed_mS * reboundCoeff;
      }
      else
      {
        // go one unit above
        z_mm = -LampTy::lampHeight_mm + LampTy::ledStripWidth_mm * 1.5;
        zSpeed_mS = -zSpeed_mS * reboundCoeff;
      }
      // udpate index
      _savedLampIndex = to_lamp_index_no_bounds();
    }
  }

  /// Bleed off some speed. Simulate a dampening factor (like friction loss)
  void dampen_speed(const float dampenFactor)
  {
    thetaSpeed_radS *= dampenFactor;
    zSpeed_mS *= dampenFactor;
  }

  /// Convert this particle coordinates to a constraint lamp index.
  uint16_t to_lamp_index() const { return to_led_index(theta_rad, z_mm); }
  /// Convert this particle coordinates to an unconstraint lamp index.
  int16_t to_lamp_index_no_bounds() const { return to_led_index_no_bounds(theta_rad, z_mm); }

  float thetaSpeed_radS; ///< angular speed, in radian/seconds
  float zSpeed_mS;       ///< linear speed, in meter/seconds

  float theta_rad; ///< position, in radians
  float z_mm;      ///< height, in millimeters.

  /// optimization
  int16_t _savedLampIndex;
};

} // namespace modes

#endif
