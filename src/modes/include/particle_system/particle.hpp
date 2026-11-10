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

namespace lampda::modes {

/**
 * \brief Define a particle in cylindrical space, and movement equations on the cylinder surface
 */
using LampTy = hardware::LampTy;

static constexpr float cylinderRadius_m = LampTy::lampBodyRadius_mm / 1000.0f; ///< radius of the cylinder, in meters
static constexpr float angularSpeedGain = 50000; ///< this gain compensate the angular acceleration for better display
static constexpr float linearSpeedGain = 1.0;    ///< gain for the linear speeds
static constexpr float reboundCoeff = 0.1f;      ///< low rebound [0 - 1] on walls
static constexpr float speedDampening = 0.92f;   ///< low speed dampening [0-0.99] at every steps (viscosity)

static constexpr float maxAngularSpeed_radS = 4 * c_TWO_PI; ///< max angular speed in radians/s
static constexpr float maxVerticalSpeed_mmS = 50;           ///< max vertical speed in mm/S

static constexpr float ledsPerTurn = LampTy::ledPerTurns; ///< leds around the tube. Not an integer: rows are sheared
static constexpr float rowHeight_mm = LampTy::ledStripWidth_mm;       ///< height gained by the helix in one turn
static constexpr float ledLength_mm = LampTy::ledSize_mm;             ///< length of a led along the strip
static constexpr float cylinderRadius_mm = LampTy::lampBodyRadius_mm; ///< radius of the cylinder, in millimeters
/// ratio between the speed gained around the tube and the physical one
static constexpr float aroundTubeRatio = angularSpeedGain * cylinderRadius_m * cylinderRadius_m * cylinderRadius_m;

/**
 * \brief Define a particle in 3D space. it has a position and speed
 */
struct Particle
{
  /// Default particle init
  Particle() = default;

  /// Copy constructor for a particle
  Particle(const Particle& other) = default;

  /// Construct a Particle from a 3D cartesian position.
  Particle(const utils::vec3d& positionCartesian) :
    theta_rad(atan2(positionCartesian.y, positionCartesian.x)),
    z_mm(positionCartesian.z)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  /// Construct a particle from an angle and height.
  Particle(const float positionTheta_rad, const float positionZ_mm) : theta_rad(positionTheta_rad), z_mm(positionZ_mm)
  {
    _savedLampIndex = to_lamp_index_no_bounds();
  }

  /**
   * \brief Compute the speed increment in the constraint lamp body
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \return The speed in angle and heigh.
   */
  utils::vec2d compute_speed_increment(const utils::vec3d& accelerationCartesian_m, const float deltaTime_s) const
  {
    // only the acceleration tangent to the cylinder surface moves the particle
    const float tangentialAcceleration =
            accelerationCartesian_m.y * fast_cos(theta_rad) - accelerationCartesian_m.x * fast_sin(theta_rad);

    return utils::vec2d(aroundTubeRatio / cylinderRadius_m * tangentialAcceleration * deltaTime_s,
                        linearSpeedGain * accelerationCartesian_m.z * deltaTime_s);
  }

  /**
   * \brief Dampen the speed and apply a cartesian acceleration to it, without moving the particle.
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \return The speed gained at this update, on the cylinder surface, in mm/s (around the tube, along the height)
   */
  utils::vec2d update_speed(const utils::vec3d& accelerationCartesian_m, const float deltaTime_s)
  {
    const auto& speedIncrement = compute_speed_increment(accelerationCartesian_m, deltaTime_s);
    // start by dampening the speed
    dampen_speed(speedDampening);

    // update speed
    thetaSpeed_radS =
            lmpd_constrain<float>(thetaSpeed_radS + speedIncrement.x, -maxAngularSpeed_radS, maxAngularSpeed_radS);
    zSpeed_mS = lmpd_constrain<float>(zSpeed_mS + speedIncrement.y, -maxVerticalSpeed_mmS, maxVerticalSpeed_mmS);

    return utils::vec2d(speedIncrement.x * cylinderRadius_mm, speedIncrement.y * 1000.0f);
  }

  /**
   * \brief apply a cartesian acceleration to this particulate
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] deltaTime_s Time since the latest update
   * \param[in] shouldContrain If true, will constrain the movement into the lamp body.
   */
  void apply_acceleration(const utils::vec3d& accelerationCartesian_m,
                          const float deltaTime_s,
                          const bool shouldContrain = true)
  {
    update_speed(accelerationCartesian_m, deltaTime_s);

    static constexpr float angularUnit = LampTy::maxWidthFloat / c_TWO_PI;
    static constexpr float verticalUnit = LampTy::ledStripWidth_mm * 1.5f;

    // update position (limit speed to pixel unit per dt)
    const float angularPositionIncrement =
            lmpd_constrain<float>(thetaSpeed_radS * deltaTime_s, -angularUnit, angularUnit);
    const float verticalPositionIncrement =
            lmpd_constrain<float>(zSpeed_mS * deltaTime_s, -verticalUnit, verticalUnit) * 1000.0f;

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
  Particle simulate_after_acceleration(const utils::vec3d& accelerationCartesian_m,
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
        z_mm = -LampTy::ledStripWidth_mm * 1.5f;
        zSpeed_mS = -zSpeed_mS * reboundCoeff;
      }
      else
      {
        // go one unit above
        z_mm = -LampTy::lampHeight_mm + LampTy::ledStripWidth_mm * 1.5f;
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

  void set_color(const uint32_t color) { compressedColor = colors::get_compressed_16b_color(color); }
  uint32_t get_color() const { return colors::get_decompressed_32b_color(compressedColor); }

  /// Convert this particle coordinates to a constraint lamp index.
  uint16_t to_lamp_index() const { return to_led_index(theta_rad, z_mm); }
  /// Convert this particle coordinates to an unconstraint lamp index: the nearest led of the helix.
  int16_t to_lamp_index_no_bounds() const { return led_index_at(theta_rad, z_mm); }

  /**
   * \brief Find the led nearest to a position, following the real helix of the strip.
   * A led height depends on its angle: choosing the turn from the height alone would be up to a full row wrong.
   * \param[in] positionTheta_rad Angle of the position, in radians.
   * \param[in] positionZ_mm Height of the position, in millimeters.
   * \return The index of the led. It is not necessarly a valid index.
   */
  static int16_t led_index_at(const float positionTheta_rad, const float positionZ_mm)
  {
    float turnFraction = positionTheta_rad * (1.0f / c_TWO_PI);
    turnFraction -= fast_floor(turnFraction);
    const float turn = fast_floor(-positionZ_mm * (1.0f / rowHeight_mm) - turnFraction + 0.5f);
    return static_cast<int16_t>(fast_floor((turn + turnFraction) * ledsPerTurn + 0.5f));
  }

  /// Return the led under this particle, once moved by an offset on the cylinder surface (in mm).
  int16_t led_index_at_offset(const utils::vec2d& offset_mm) const
  {
    return led_index_at(theta_rad + offset_mm.x * (1.0f / cylinderRadius_mm), z_mm + offset_mm.y);
  }

  /**
   * \brief Potential energy of a led under an acceleration, consistent with the acceleration of a particle.
   * \param[in] accelerationCartesian_m Acceleration applied at this update
   * \param[in] ledIndex Index of the led. It can be out of the strip.
   * \return The potential (a particle is accelerated toward the leds with a lower one), in mm.m/s^2
   */
  static float potential_at_led(const utils::vec3d& accelerationCartesian_m, const int16_t ledIndex)
  {
    const float angle_rad = ledIndex * (c_TWO_PI / ledsPerTurn);
    return -aroundTubeRatio * cylinderRadius_mm *
                   (accelerationCartesian_m.x * fast_cos(angle_rad) + accelerationCartesian_m.y * fast_sin(angle_rad)) -
           accelerationCartesian_m.z * to_helix_z(ledIndex);
  }

  /// Return the speed on the cylinder surface, in mm/s (around the tube, along the height)
  utils::vec2d get_surface_speed() const
  {
    return utils::vec2d(thetaSpeed_radS * cylinderRadius_mm, zSpeed_mS * 1000.0f);
  }

  /// Move this particle on the cylinder surface, to a led already computed from this offset.
  void move_by(const utils::vec2d& offset_mm, const int16_t ledIndex)
  {
    theta_rad += offset_mm.x * (1.0f / cylinderRadius_mm);
    // offsets are small, so one turn is enough to stay in the 0 to 2*PI range
    if (theta_rad < 0.0f)
      theta_rad += c_TWO_PI;
    else if (theta_rad >= c_TWO_PI)
      theta_rad -= c_TWO_PI;
    z_mm += offset_mm.y;
    _savedLampIndex = ledIndex;
  }

  float thetaSpeed_radS = 0.0f; ///< angular speed, in radian/seconds
  float zSpeed_mS = 0.0f;       ///< linear speed, in meter/seconds

  float theta_rad = 0.0f; ///< position, in radians
  float z_mm = 0.0f;      ///< height, in millimeters.

  /// optimization
  int16_t _savedLampIndex;

private:
  /// Floor for values far below the int limit. The collisions call it a lot, and it is much cheaper than a libm call.
  static float fast_floor(const float value)
  {
    const float truncated = static_cast<float>(static_cast<int32_t>(value));
    return (truncated > value) ? truncated - 1.0f : truncated;
  }

  /// Parabolic sine, about 5% off at worst. It drives a display animation, and has no division unlike sin_t.
  static float fast_sin(const float angle_rad)
  {
    float turns = angle_rad * (1.0f / c_TWO_PI);
    turns -= fast_floor(turns + 0.5f);
    return 8.0f * turns - 16.0f * turns * fabsf(turns);
  }

  /// Parabolic cosine, see fast_sin
  static float fast_cos(const float angle_rad) { return fast_sin(angle_rad + c_HALF_PI); }

  uint16_t compressedColor = colors::get_compressed_16b_color(colors::HTMLColorCode::White); ///< stored particle color
};

} // namespace lampda::modes

#endif
