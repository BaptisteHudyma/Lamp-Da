/*! \file particle_system.hpp
    \brief Define a particle system, with particle based logic.
*/

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <cstdint>
#include <functional>
#include <set>

#include "src/system/ext/random8.h"

#include "src/modes/include/hardware/lamp_type.hpp"

#include "particle.hpp"

namespace lampda::modes {

using LampTy = hardware::LampTy;

/**
 * \brief Define a particle system
 * ALL PARTICLE SYSTEM SHARE THE SAME PÄRTICLE SUBSET
 */
class ParticleSystem
{
public:
  ParticleSystem() : particuleCount(0) { reset(); }

  /**
   * \brief  reset system to zero count
   */
  void reset()
  {
    occupiedSpacesSet.clear();
    for (size_t i = 0; i < ParticleSystem::maxParticuleCount; ++i)
    {
      isAllocated[i] = false;
    }
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
  void init_particules(const std::function<int16_t(size_t)>& positionGeneratorFunction)
  {
    occupiedSpacesSet.clear();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      isAllocated[i] = false;
      spawn_particule(i, positionGeneratorFunction);
    }
  }

  /**
   * \brief Init the particles from a initialization function, in a timed defered way.
   * \param[in] maxParticlesToPop Maximum particles to spawn at this call
   * \param[in] positionGeneratorFunction Function that takes an index and return a led strip index
   */
  void init_deferred_particules(uint8_t maxParticlesToPop,
                                const std::function<int16_t(size_t)>& positionGeneratorFunction)
  {
    for (size_t i = 0; i < particuleCount and maxParticlesToPop > 0; ++i)
    {
      if (isAllocated[i])
      {
        // already allocated, do not reuse
        continue;
      }
      spawn_particule(i, positionGeneratorFunction);
      maxParticlesToPop--;
    }
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
   * \param[in] accelerationCartesian 3d acceleration vector to apply to particles
   * \param[in] deltaTime_s Time since last update, in seconds
   * \param[in] shouldContrain If true, will constrain the particles to the lamp body.
   */
  void iterate_with_collisions(const utils::vec3d& accelerationCartesian,
                               const float deltaTime_s,
                               const bool shouldContrain = true)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not iterate non allocated particles
      if (not isAllocated[i])
        continue;

      Particle& p = particules[i];
      const int16_t ledIndex = p._savedLampIndex;

      // simulate instead of updating directly
      const Particle& newP = p.simulate_after_acceleration(accelerationCartesian, deltaTime_s, shouldContrain);

      // update particle position in occupation set
      const int16_t newLedIndex = newP._savedLampIndex;
      if (newLedIndex != ledIndex)
      {
        // check collision : no collision
        if (not is_position_taken(newLedIndex))
        {
          occupiedSpacesSet.erase(ledIndex);
          occupiedSpacesSet.insert(newLedIndex);
        }
        // check collision : collision !!
        else
        {
          // refuse movement, rebound speed
          p.thetaSpeed_radS = -p.thetaSpeed_radS * 0.75;
          p.zSpeed_mS = -p.zSpeed_mS * 0.75;
          continue;
        }
      }

      // update particle
      p = newP;
    }
  }

  /**
   * \brief Filter particles depending on condition
   * \param[in] shouldDepopFunction A function, that given a particle, will return true if it needs to be removed from
   * the simulation.
   * \return The number of particles left in the simulation.
   */
  uint16_t depop_particules(const std::function<bool(const Particle&)>& shouldDepopFunction)
  {
    uint16_t particleCount = 0;
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not depop non allocated particles
      if (not isAllocated[i])
        continue;

      auto& parti = particules[i];
      if (shouldDepopFunction(parti))
      {
        isAllocated[i] = false;
        occupiedSpacesSet.erase(parti._savedLampIndex);
      }
      else
      {
        particleCount += 1;
      }
    }
    return particleCount;
  }

  /**
   * \brief Display this particle system
   * \param[in] sample_color sampling function for the particle color
   * \param[in, out] lamp The lamp object to write to
   */
  uint16_t show(const std::function<uint32_t(int16_t, const Particle&)> sample_color, LampTy& lamp)
  {
    uint16_t particleCount = 0;
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not show non allocated particles
      if (not isAllocated[i])
        continue;
      const auto& particle = particules[i];
      const auto& index = particle._savedLampIndex;
      if (modes::is_led_index_valid(index))
        lamp.setPixelColor(index, sample_color(i, particle));
      else
        particleCount += 1;
    }
    return particleCount;
  }

protected:
  /// Return True if the position is already occupied
  bool is_position_taken(const int16_t pos) const { return occupiedSpacesSet.find(pos) != occupiedSpacesSet.cend(); }

  /**
   * \brief Spawn a particle
   * \param[in] index Index of this particle
   * \param[in] positionGeneratorFunction Spawn function
   */
  void spawn_particule(const size_t index, const std::function<int16_t(size_t)>& positionGeneratorFunction)
  {
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
    occupiedSpacesSet.insert(pos);
    isAllocated[index] = true;
  }

private:
  static constexpr uint16_t maxParticuleCount = 512; ///< maximum particles allowed in a simulation
  Particle particules[maxParticuleCount];            ///< Array of all particles
  bool isAllocated[maxParticuleCount];               ///< store the allocated particules flag

  std::set<int16_t> occupiedSpacesSet; ///< store the occupied spaces

  /// forced to be less than maxParticuleCount
  uint16_t particuleCount;
};

} // namespace lampda::modes

#endif
