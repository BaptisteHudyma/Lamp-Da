/*! \file particle_system.hpp
    \brief Define a particle system, with particle based logic.
*/

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <etl/bitset.h>
#include <initializer_list>

#include "src/system/ext/random8.h"

#include "src/modes/include/hardware/lamp_type.hpp"

#include "particle.hpp"

namespace lampda::modes {

using LampTy = hardware::LampTy;

/**
 * \brief Define a particle system
 * ALL PARTICLE SYSTEM SHARE THE SAME PARTICLE SUBSET
 */
template<size_t MaxParticuleCount> class ParticleSystem
{
public:
  ParticleSystem() : particuleCount(0) { reset(); }
  ParticleSystem(const ParticleSystem&) = delete;

  /**
   * \brief  reset system to zero count
   */
  void reset()
  {
    isOccupiedSpaces.reset();
    isAllocated.reset();
    isResting.reset();
  }

  /**
   * \brief Call when you want to change the particle count of the simulation
   * Can be called once when starting the particle system
   */
  void set_max_particle_count(const uint16_t _particleCount)
  {
    particuleCount = std::min<uint16_t>(ParticleSystem::maxParticuleCount, _particleCount);
    reset();
  }

  /**
   * \brief Init the particles from a initialization function
   * \param[in] positionGeneratorFunction Function that takes an index and return a ledstrip index
   */
  template<typename PositionGeneratorFunction>
  void init_particules(const PositionGeneratorFunction& positionGeneratorFunction,
                       const uint32_t color = colors::HTMLColorCode::White)
  {
    static_assert(std::is_invocable_r_v<int16_t, PositionGeneratorFunction, size_t>);

    isOccupiedSpaces.reset();
    isAllocated.reset();
    isResting.reset();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      spawn_particule(i, positionGeneratorFunction, color);
    }
  }

  /**
   * \brief Init the particles from a initialization function, in a timed defered way.
   * \param[in] maxParticlesToPop Maximum particles to spawn at this call
   * \param[in] positionGeneratorFunction Function that takes an index and return a led strip index
   */
  template<typename PositionGeneratorFunction>
  bool init_deferred_particules(uint8_t maxParticlesToPop, const PositionGeneratorFunction& positionGeneratorFunction)
  {
    static_assert(std::is_invocable_r_v<int16_t, PositionGeneratorFunction, size_t>);

    const auto colorGeneratorFunction = [](const size_t) -> uint32_t {
      return colors::HTMLColorCode::White;
    };
    return init_deferred_particules(maxParticlesToPop, positionGeneratorFunction, colorGeneratorFunction);
  }

  /**
   * \brief Init the particles from a initialization function, in a timed defered way.
   * \param[in] maxParticlesToPop Maximum particles to spawn at this call
   * \param[in] positionGeneratorFunction Function that takes an index and return a led strip index
   * \param[in] colorGeneratorFunction Function that takes an index and return a color of the particle at this index
   */
  template<typename PositionGeneratorFunction, typename ColorGeneratorFunction>
  bool init_deferred_particules(uint8_t maxParticlesToPop,
                                const PositionGeneratorFunction& positionGeneratorFunction,
                                const ColorGeneratorFunction& colorGeneratorFunction)
  {
    static_assert(std::is_invocable_r_v<int16_t, PositionGeneratorFunction, size_t>);
    static_assert(std::is_invocable_r_v<uint32_t, ColorGeneratorFunction, size_t>);

    for (size_t i = 0; i < particuleCount and maxParticlesToPop > 0; ++i)
    {
      if (isAllocated[i])
      {
        // already allocated, do not reuse
        continue;
      }
      spawn_particule(i, positionGeneratorFunction, colorGeneratorFunction(i));
      maxParticlesToPop--;
    }
    // still some uncreated particles
    return maxParticlesToPop > 0;
  }

  /**
   * \brief Advance the particle simulation, ignoring collisions
   * \param[in] accelerationCartesian 3d acceleration vector to apply to particles
   * \param[in] deltaTime_s Time since last update, in seconds
   * \param[in] shouldContrain If true, will constrain the particles to the lamp body.
   */
  void iterate_no_collisions(const utils::vec3d& accelerationCartesian,
                             const float deltaTime_s,
                             const bool shouldContrain = true)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not iterate non allocated particles
      if (not isAllocated[i])
        continue;

      Particle& p = particules[i];
      // apply force and constrain
      p.apply_acceleration(accelerationCartesian, deltaTime_s, shouldContrain);
    }
  }

  /**
   * \brief Advance the particle simulation, taking collisions in account.
   * A led holds one particle, and a blocked particle flows around the obstacle. Resting particles update less often.
   * \param[in] accelerationCartesian 3d acceleration vector to apply to particles
   * \param[in] deltaTime_s Time since last update, in seconds
   * \param[in] shouldContrain If true, will constrain the particles to the lamp body.
   * \param[in] viscosity From 0 (liquid, levels itself) to 1 (sand, forms piles)
   */
  void iterate_with_collisions(const utils::vec3d& accelerationCartesian,
                               const float deltaTime_s,
                               const bool shouldContrain = true,
                               const float viscosity = sandViscosity)
  {
    update_occupation();

    frameIndex += 1;
    const uint8_t flowReach = 1 + static_cast<uint8_t>((1.0f - viscosity) * maxFlowReach + 0.5f);

    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not iterate non allocated particles
      if (not isAllocated[i])
        continue;

      // spread the resting particles over the frames. A support that disappears is seen a few frames late, at worst
      if (isResting[i] and ((i + frameIndex) % restingUpdatePeriod) != 0)
        continue;

      const utils::vec2d& drive = particules[i].update_speed(accelerationCartesian, deltaTime_s);
      move_with_collisions(i, accelerationCartesian, drive, deltaTime_s, shouldContrain, flowReach);
    }
  }

  /**
   * \brief Filter particles depending on condition
   * \param[in] shouldDepopFunction A function, that given a particle, will return true if it needs to be removed from
   * the simulation.
   * \return The number of particles left in the simulation.
   */
  template<typename ShouldDepopFunction> uint16_t depop_particules(const ShouldDepopFunction& shouldDepopFunction)
  {
    static_assert(std::is_invocable_r_v<bool, ShouldDepopFunction, Particle const&>);

    uint16_t particleCount = 0;
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not depop non allocated particles
      if (not isAllocated[i])
        continue;

      const auto& parti = particules[i];
      if (shouldDepopFunction(parti))
      {
        isAllocated[i] = false;
        if (parti._savedLampIndex >= 0 && parti._savedLampIndex < isOccupiedSpaces.size())
          isOccupiedSpaces[parti._savedLampIndex] = false;
      }
      else
      {
        particleCount += 1;
      }
    }
    return particleCount;
  }

  /**
   * \brief Same as depop_particules, but the predicate also receives the particle slot index.
   * \return The number of particles left in the simulation.
   */
  template<typename ShouldDepopFunction>
  uint16_t depop_particules_indexed(const ShouldDepopFunction& shouldDepopFunction)
  {
    static_assert(std::is_invocable_r_v<bool, ShouldDepopFunction, size_t, Particle const&>);

    uint16_t remaining = 0;
    for (size_t i = 0; i < particuleCount; ++i)
    {
      if (not isAllocated[i])
        continue;

      const auto& parti = particules[i];
      if (shouldDepopFunction(i, parti))
      {
        isAllocated[i] = false;
        if (parti._savedLampIndex >= 0 && parti._savedLampIndex < isOccupiedSpaces.size())
          isOccupiedSpaces[parti._savedLampIndex] = false;
      }
      else
      {
        remaining += 1;
      }
    }
    return remaining;
  }

  /**
   * \brief Display this particle system
   * \param[in] sample_color sampling function for the particle color
   * \param[in, out] lamp The lamp object to write to
   */
  template<typename SampleColorFunction>
  uint16_t show(const SampleColorFunction& sample_color, LampTy& lamp, const bool forceDisplay = false) const
  {
    static_assert(std::is_invocable_r_v<uint32_t, SampleColorFunction, int16_t, Particle const&>);

    uint16_t particleCount = 0;
    for (int16_t i = 0; i < particuleCount; ++i)
    {
      // do not show non allocated particles
      if (not isAllocated[i])
        continue;
      const auto& particle = particules[i];
      const auto& index = particle._savedLampIndex;
      if (modes::is_led_index_valid(index))
      {
        if (forceDisplay)
          lamp.mask_led_at_index(index, false);
        lamp.setPixelColor(index, sample_color(i, particle));
      }
      else
        particleCount += 1;
    }
    return particleCount;
  }

  uint16_t show(LampTy& lamp, const bool forceDisplay = false) const
  {
    return show(
            [](int16_t, const Particle& particle) {
              return particle.get_color();
            },
            lamp,
            forceDisplay);
  }

  /// Return the number of active particles
  uint16_t get_number_of_active() const
  {
    uint16_t particleCount = 0;
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not show non allocated particles
      if (not isAllocated[i])
        continue;
      particleCount += 1;
    }
    return particleCount;
  }

protected:
  /// Return True if the position is already occupied
  bool is_position_taken(const int16_t pos) const
  {
    if (pos < 0 or pos >= isOccupiedSpaces.size())
      return false;
    return isOccupiedSpaces[pos];
  }

  /**
   * \brief Spawn a particle
   * \param[in] index Index of this particle
   * \param[in] positionGeneratorFunction Spawn function
   */
  template<typename PositiongeneratorFunction>
  void spawn_particule(const size_t index,
                       const PositiongeneratorFunction& positionGeneratorFunction,
                       const uint32_t color)
  {
    static_assert(std::is_invocable_r_v<int16_t, PositiongeneratorFunction, size_t>);

    int16_t pos = positionGeneratorFunction(index);
    int maxTries = 3;
    while (ParticleSystem::is_position_taken(pos) and maxTries > 0)
    {
      pos = positionGeneratorFunction(index);
      maxTries--;
    }
    // generate start position from user function
    const auto& helixCoordinates = modes::strip_to_helix_unconstraint(pos);
    particules[index] = Particle(utils::vec3d {helixCoordinates.x, helixCoordinates.y, helixCoordinates.z});
    particules[index].set_color(color);
    if (index < isAllocated.size())
    {
      isAllocated[index] = true;
      isResting[index] = false;
    }
    const int16_t ledIndex = particules[index]._savedLampIndex;
    if (ledIndex >= 0 && ledIndex < isOccupiedSpaces.size())
      isOccupiedSpaces[ledIndex] = true;
  }

private:
  static constexpr float sandViscosity = 0.75f;    ///< default viscosity: grains that pile up, then slowly slump
  static constexpr float contactDampening = 0.5f;  ///< part of the speed kept by a blocked particle
  static constexpr float minFlowDropRatio = 0.25f; ///< smallest drop that can make a particle flow, in led heights
  static constexpr float maxCollisionSpeed_mmS = 1300.0f; ///< fastest move of a colliding particle
  static constexpr uint8_t maxFlowReach = 12;             ///< leds scanned sideways for a gap, at zero viscosity
  static constexpr uint8_t restingUpdatePeriod = 4;       ///< a resting particle updates once in this number of frames

  /// Rebuild the occupied leds from the allocated particles
  void update_occupation()
  {
    isOccupiedSpaces.reset();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      const int16_t ledIndex = particules[i]._savedLampIndex;
      if (isAllocated[i] and modes::is_led_index_valid(ledIndex))
        isOccupiedSpaces[ledIndex] = true;
    }
  }

  /// Return true if a particle on the led \p fromIndex can not enter the led \p toIndex
  bool is_led_blocked(const int16_t fromIndex, const int16_t toIndex, const bool shouldContrain) const
  {
    if (modes::is_led_index_valid(toIndex))
      return isOccupiedSpaces[toIndex];
    // the strip ends act as walls. Let a particle that is out of the strip get back in
    return shouldContrain and modes::is_led_index_valid(fromIndex);
  }

  /// Move a particle and keep the occupied leds up to date
  void move_particle(const size_t slot, const utils::vec2d& offset_mm, const int16_t toIndex)
  {
    Particle& particle = particules[slot];
    const int16_t fromIndex = particle._savedLampIndex;
    if (toIndex != fromIndex)
    {
      // a particle sharing the old led makes it look free until the next update_occupation: this is only visual
      if (modes::is_led_index_valid(fromIndex))
        isOccupiedSpaces[fromIndex] = false;
      if (modes::is_led_index_valid(toIndex))
        isOccupiedSpaces[toIndex] = true;
    }
    particle.move_by(offset_mm, toIndex);
  }

  /**
   * \brief Move a particle at its speed, or let it flow if the destination is blocked.
   * There is a single collision check by update: a fast particle can jump over a led.
   * \param[in] slot Index of the particle
   * \param[in] acceleration 3d acceleration vector applied to the particles
   * \param[in] drive Speed gained from the acceleration at this update
   * \param[in] deltaTime_s Time since last update, in seconds
   * \param[in] shouldContrain If true, the strip ends are walls
   * \param[in] flowReach Leds scanned sideways for a gap on contact
   */
  void move_with_collisions(const size_t slot,
                            const utils::vec3d& acceleration,
                            const utils::vec2d& drive,
                            const float deltaTime_s,
                            const bool shouldContrain,
                            const uint8_t flowReach)
  {
    Particle& particle = particules[slot];

    utils::vec2d step = particle.get_surface_speed().multiply(deltaTime_s);
    const float longestAxis_mm = std::max(fabsf(step.x), fabsf(step.y));
    const float maxStep_mm = maxCollisionSpeed_mmS * deltaTime_s;
    if (longestAxis_mm > maxStep_mm)
      step = step.multiply(maxStep_mm / longestAxis_mm);

    const int16_t fromIndex = particle._savedLampIndex;
    const int16_t toIndex = particle.led_index_at_offset(step);
    if (toIndex == fromIndex or not is_led_blocked(fromIndex, toIndex, shouldContrain))
    {
      move_particle(slot, step, toIndex);
      isResting[slot] = false;
      return;
    }

    particle.dampen_speed(contactDampening);
    // only the acceleration can make a particle flow, or the rebounds would agitate the piles forever
    const bool hasFlowed =
            drive.dot(step) > 0.0f and flow_around_obstacle(slot, acceleration, drive, shouldContrain, flowReach);
    isResting[slot] = not hasFlowed;
  }

  /**
   * \brief Let a blocked particle flow to a free led that is lower than itself, along the acceleration.
   * Each flow lowers the potential energy of the particle, so the piles always come to rest.
   * Candidates are found from positions and not from led index offsets, as the rows of leds are sheared and not square.
   * \param[in] slot Index of the particle
   * \param[in] acceleration 3d acceleration vector applied to the particles
   * \param[in] drive Speed gained from the acceleration at this update
   * \param[in] shouldContrain If true, the strip ends are walls
   * \param[in] flowReach Leds scanned sideways for a gap. 1 only checks the diagonals, like sand.
   * \return true if the particle moved
   */
  bool flow_around_obstacle(const size_t slot,
                            const utils::vec3d& acceleration,
                            const utils::vec2d& drive,
                            const bool shouldContrain,
                            const uint8_t flowReach)
  {
    const Particle& particle = particules[slot];
    const int16_t fromIndex = particle._savedLampIndex;

    const utils::vec2d& downhill = drive.multiply(1.0f / sqrtf(drive.dot(drive)));
    const utils::vec2d& oneLedDown = downhill.multiply(led_span_along(downhill));
    const utils::vec2d sideDirection(-downhill.y, downhill.x);
    const utils::vec2d& oneLedAside = sideDirection.multiply(led_span_along(sideDirection));

    // no side preference, or the piles would lean over time
    const float firstSide = (random8() & 1) ? 1.0f : -1.0f;
    for (uint8_t distance = 1; distance <= flowReach; ++distance)
    {
      // only the surface flows far: it spares the scans for the many particles buried in a pile
      if (distance == 2 and
          is_led_blocked(fromIndex, particle.led_index_at_offset(oneLedDown.multiply(-1.0f)), shouldContrain))
        return false;

      for (const float side: {firstSide, -firstSide})
      {
        const utils::vec2d& gapOffset = oneLedAside.multiply(side * distance).add(oneLedDown);
        const int16_t gapIndex = particle.led_index_at_offset(gapOffset);
        if (gapIndex != fromIndex and not is_led_blocked(fromIndex, gapIndex, shouldContrain) and
            is_flow_worth_it(acceleration, fromIndex, gapIndex))
        {
          move_particle(slot, gapOffset, gapIndex);
          return true;
        }
      }
    }
    return false;
  }

  /// Return true if going between these leds lowers the particle by more than a fraction of a led.
  /// The margin keeps the particles still on a level surface.
  static bool is_flow_worth_it(const utils::vec3d& acceleration, const int16_t fromIndex, const int16_t toIndex)
  {
    const float minDrop = minFlowDropRatio * rowHeight_mm * sqrtf(acceleration.dot(acceleration));
    return Particle::potential_at_led(acceleration, toIndex) <
           Particle::potential_at_led(acceleration, fromIndex) - minDrop;
  }

  /// Return the distance to the next led along a unit vector: the leds are longer than they are high (or the opposite)
  static float led_span_along(const utils::vec2d& direction)
  {
    return fabsf(direction.x) * ledLength_mm + fabsf(direction.y) * rowHeight_mm;
  }

  static constexpr uint16_t maxParticuleCount = MaxParticuleCount; ///< maximum particles allowed in a simulation
  std::array<Particle, maxParticuleCount> particules;              ///< Array of all particles
  etl::bitset<maxParticuleCount> isAllocated;                      ///< store the allocated particules flag
  etl::bitset<maxParticuleCount> isResting;                        ///< store the blocked particules flag
  etl::bitset<LampTy::ledCount> isOccupiedSpaces;                  ///< store the occupied spaces
  uint8_t frameIndex = 0;                                          ///< spreads the resting particles over the frames

  /// forced to be less than maxParticuleCount
  uint16_t particuleCount;
};

} // namespace lampda::modes

#endif
