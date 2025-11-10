#ifdef LMBD_LAMP_TYPE__INDEXABLE

#ifndef PARTICLE_H
#define PARTICLE_H

#include "src/system/utils/vector_math.h"
#include "src/system/utils/coordinates.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/print.h"

/**
 * \brief Define a particle in cylindrical space, and movement equations on the cylinder surface
 */

static constexpr float cylinderRadius_m = lampBodyRadius_mm / 1000.0;
static constexpr float angularSpeedGain = 50000; // this gain compensate the angular acceleration for better display
static constexpr float linearSpeedGain = 1.0;
static constexpr float reboundCoeff = 0.1;    // low rebound [0 - 1] on walls
static constexpr float speedDampening = 0.92; // low speed dampening [0-0.99] at every steps (viscosity)

static constexpr float maxAngularSpeed_radS = 4 * c_TWO_PI;
static constexpr float maxVerticalSpeed_mmS = 50;

struct Particle
{
  Particle() : thetaSpeed_radS(0.0), zSpeed_mS(0.0), theta_rad(0.0), z_mm(0.0) {}

  Particle(const vec3d& positionCartesian) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(atan2(positionCartesian.y, positionCartesian.x)),
    z_mm(positionCartesian.z)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  Particle(const float positionTheta_rad, const float positionZ_mm) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(positionTheta_rad),
    z_mm(positionZ_mm)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  Particle(const Particle& other) :
    thetaSpeed_radS(other.thetaSpeed_radS),
    zSpeed_mS(other.zSpeed_mS),
    theta_rad(other.theta_rad),
    z_mm(other.z_mm),
    _savedLampIndex(other._savedLampIndex)
  {
  }

  vec2d compute_speed_increment(const vec3d& accelerationCartesian_m, const float delaTime) const
  {
    // speed vector on radial (derivative of cartesian to cylinder coordinates for theta)
    const vec3d e_theta(-sin(theta_rad) * cylinderRadius_m, cos(theta_rad) * cylinderRadius_m, 0);
    // speed vector on z (derivative of cartesian to cylinder coordinates for z)
    const vec3d e_z(0, 0, 1);
    // ignore the radius derivative, as we want to stay on the cylinder surface

    // compute the speed vector on cylinder surface
    const vec3d& tangantialVector = e_theta.multiply(accelerationCartesian_m.dot(e_theta));
    const vec3d& directVector = e_z.multiply(accelerationCartesian_m.dot(e_z));
    const vec3d& accelerationVector = tangantialVector.add(directVector);

    return vec2d(angularSpeedGain * accelerationVector.dot(e_theta) / static_cast<float>(cylinderRadius_m) * delaTime,
                 linearSpeedGain * accelerationVector.dot(e_z) * delaTime);
  }

  /**
   * \brief apply a cartesian acceleration to this particulate
   */
  void apply_acceleration(const vec3d& accelerationCartesian_m, const float delaTime, const bool shouldContrain = true)
  {
    const auto& speedIncrement = compute_speed_increment(accelerationCartesian_m, delaTime);
    // start by dampening the speed
    dampen_speed(speedDampening);

    // update speed
    thetaSpeed_radS =
            lmpd_constrain<float>(thetaSpeed_radS + speedIncrement.x, -maxAngularSpeed_radS, maxAngularSpeed_radS);
    zSpeed_mS = lmpd_constrain<float>(zSpeed_mS + speedIncrement.y, -maxVerticalSpeed_mmS, maxVerticalSpeed_mmS);

    static constexpr float angularUnit = stripXCoordinates / c_TWO_PI;
    static constexpr float verticalUnit = ledStripWidth_mm * 1.5;

    // update position (limit speed to pixel unit per dt)
    const float angularPositionIncrement = lmpd_constrain<float>(thetaSpeed_radS * delaTime, -angularUnit, angularUnit);
    const float verticalPositionIncrement =
            lmpd_constrain<float>(zSpeed_mS * delaTime, -verticalUnit, verticalUnit) * 1000.0;

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

  Particle simulate_after_acceleration(const vec3d& accelerationCartesian_m,
                                       const float delaTime,
                                       const bool shouldContrain = true) const
  {
    Particle simulated(*this);
    simulated.apply_acceleration(accelerationCartesian_m, delaTime, shouldContrain);
    return simulated;
  }

  /**
   * \brief Contrain the particle movement to the cylinder, limiting the movement at the extremities
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

    if (z_mm < -lampHeight)
    {
      z_mm = -lampHeight;
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
        z_mm = ledStripWidth_mm * 1.5;
        zSpeed_mS = -zSpeed_mS * reboundCoeff;
      }
      else
      {
        // go one unit above
        z_mm = -lampHeight + ledStripWidth_mm * 1.5;
        zSpeed_mS = -zSpeed_mS * reboundCoeff;
      }
      // udpate index
      _savedLampIndex = to_lamp_index_no_bounds();
    }
  }

  void dampen_speed(const float dampenFactor)
  {
    thetaSpeed_radS *= dampenFactor;
    zSpeed_mS *= dampenFactor;
  }

  uint16_t to_lamp_index() const { return to_led_index(theta_rad, z_mm); }
  int16_t to_lamp_index_no_bounds() const { return to_led_index_no_bounds(theta_rad, z_mm); }

  float thetaSpeed_radS;
  float zSpeed_mS;

  float theta_rad;
  float z_mm;

  // optimization
  int16_t _savedLampIndex;
};

#endif

#endif
