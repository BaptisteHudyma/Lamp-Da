#ifndef PARTICULE_CYLINDER_H
#define PARTICULE_CYLINDER_H

#include "src/system/utils/vector_math.h"
#include "src/system/utils/coordinates.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/utils/print.h"

/**
 * \brief Define a particule in cylindrical space, and movement equations on the cylinder surface
 */

static constexpr float cylinderRadius_m = lampBodyRadius_mm / 1000.0;
static constexpr float angularSpeedGain = 50000; // this gain compensate the angular acceleration for better display
static constexpr float reboundCoeff = 0.1;
static constexpr float speedDampening = 0.99;

struct Particulate
{
  Particulate() : thetaSpeed_radS(0.0), zSpeed_mS(0.0), theta_rad(0.0), z_mm(0.0) {}

  Particulate(const vec3d& positionCartesian) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(atan2(positionCartesian.y, positionCartesian.x)),
    z_mm(positionCartesian.z)
  {
  }

  Particulate(const float positionTheta_rad, const float positionZ_mm) :
    thetaSpeed_radS(0.0),
    zSpeed_mS(0.0),
    theta_rad(positionTheta_rad),
    z_mm(positionZ_mm)
  {
  }

  Particulate(const Particulate& other) :
    thetaSpeed_radS(other.thetaSpeed_radS),
    zSpeed_mS(other.zSpeed_mS),
    theta_rad(other.theta_rad),
    z_mm(other.z_mm)
  {
  }

  vec2d compute_speed_increment(const vec3d& accelerationCartesian_m, const float delaTime) const
  {
    // speed vector on radial
    const vec3d e_theta(-sin(theta_rad) * cylinderRadius_m, cos(theta_rad) * cylinderRadius_m, 0);
    // speed vector on z
    const vec3d e_z(0, 0, 1);

    // compute the speed vector on cylinder surface
    const vec3d& tangantialVector = e_theta.multiply(accelerationCartesian_m.dot(e_theta));
    const vec3d& directVector = e_z.multiply(accelerationCartesian_m.dot(e_z));
    const vec3d& accelerationVector = tangantialVector.add(directVector);

    return vec2d(angularSpeedGain * accelerationVector.dot(e_theta) / static_cast<float>(cylinderRadius_m) * delaTime,
                 accelerationVector.dot(e_z) * delaTime);
  }

  /**
   * \brief apply a cartesian acceleration to this particulate
   */
  void apply_acceleration(const vec3d& accelerationCartesian_m, const float delaTime)
  {
    const auto& speedIncrement = compute_speed_increment(accelerationCartesian_m, delaTime);
    // start by dampening the speed
    dampen_speed(speedDampening);

    // update speed
    thetaSpeed_radS += speedIncrement.x;
    zSpeed_mS += speedIncrement.y;

    // update position (limit speed to 1px per dt)
    theta_rad +=
            lmpd_constrain(thetaSpeed_radS * delaTime, -stripXCoordinates / c_TWO_PI, stripXCoordinates / c_TWO_PI);
    z_mm += (lmpd_constrain(zSpeed_mS * delaTime, -ledStripWidth_mm, ledStripWidth_mm)) * 1000.0;

    // limit the derivation of the angle
    theta_rad = wrap_angle(theta_rad);
  }

  Particulate simulate_after_acceleration(const vec3d& accelerationCartesian_m, const float delaTime) const
  {
    Particulate simulated(*this);
    simulated.apply_acceleration(accelerationCartesian_m, delaTime);
    return simulated;
  }

  /**
   * \brief Contrain the particule movement to the cylinder, limiting the movement at the extremities
   */
  void constraint_into_lamp_body()
  {
    // coordinates go from 0 to -max
    if (z_mm > 0)
    {
      z_mm = 0;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
    }

    if (z_mm < -lampHeight)
    {
      z_mm = -lampHeight;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
    }

    // handle the real limits (led strip do not start and end at zero depth)
    if (z_mm >= 0 and is_lamp_coordinate_out_of_bounds(theta_rad, z_mm))
    {
      z_mm = ledStripWidth_mm * 1.5;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
    }
    if (z_mm <= -lampHeight and is_lamp_coordinate_out_of_bounds(theta_rad, z_mm))
    {
      z_mm = -lampHeight + ledStripWidth_mm * 1.5;
      zSpeed_mS = -zSpeed_mS * reboundCoeff;
    }
  }

  void dampen_speed(const float dampenFactor)
  {
    thetaSpeed_radS *= dampenFactor;
    zSpeed_mS *= dampenFactor;
  }

  uint16_t to_lamp_index() const { return to_led_index(theta_rad, z_mm); }

  float thetaSpeed_radS;
  float zSpeed_mS;

  float theta_rad;
  float z_mm;
};

#endif