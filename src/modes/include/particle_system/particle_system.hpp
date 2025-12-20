#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <cstdint>
#include <functional>
#include <set>

#include "src/system/ext/random8.h"

#include "src/modes/include/hardware/lamp_type.hpp"

#include "particle.hpp"

namespace modes {

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
    particuleCount = min<uint16_t>(ParticleSystem::maxParticuleCount, _particleCount);
    reset();
  }

  void init_particules(const std::function<int16_t(size_t)>& positionGeneratorFunction)
  {
    occupiedSpacesSet.clear();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      isAllocated[i] = false;
      spawn_particule(i, positionGeneratorFunction);
    }
  }

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

  void iterate_no_collisions(const vec3d& accelerationCartesian,
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

  void iterate_with_collisions(const vec3d& accelerationCartesian,
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
      Particle newP = p.simulate_after_acceleration(accelerationCartesian, deltaTime_s, shouldContrain);

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
  bool is_position_taken(const int16_t pos) const { return occupiedSpacesSet.find(pos) != occupiedSpacesSet.cend(); }

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
    particules[index] = Particle(vec3d {helixCoordinates.x, helixCoordinates.y, helixCoordinates.z});
    occupiedSpacesSet.insert(pos);
    isAllocated[index] = true;
  }

private:
  static constexpr uint16_t maxParticuleCount = 512;
  Particle particules[maxParticuleCount];
  bool isAllocated[maxParticuleCount]; // store the allocated particules flag

  std::set<int16_t> occupiedSpacesSet; // store the occupied spaces

  // forced to be less than maxParticuleCount
  uint16_t particuleCount;
};

} // namespace modes

#endif
