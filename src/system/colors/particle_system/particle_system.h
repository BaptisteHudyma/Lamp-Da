#ifdef LMBD_LAMP_TYPE__INDEXABLE

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <cstdint>
#include <functional>
#include <set>

#include "src/system/ext/random8.h"
#include "src/system/colors/colors.h"

#include "src/system/colors/particle_system/particle.h"

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
                             const float deltaTime,
                             const bool shouldContrain = true)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not iterate non allocated particles
      if (not isAllocated[i])
        continue;

      Particle& p = particules[i];
      // apply force and constrain
      p.apply_acceleration(accelerationCartesian, deltaTime, shouldContrain);
    }
  }

  void iterate_with_collisions(const vec3d& accelerationCartesian,
                               const float deltaTime,
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
      Particle newP = p.simulate_after_acceleration(accelerationCartesian, deltaTime, shouldContrain);

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
  void depop_particules(const std::function<bool(const Particle&)>& positionGeneratorFunction)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not depop non allocated particles
      if (not isAllocated[i])
        continue;

      auto& parti = particules[i];
      if (positionGeneratorFunction(parti))
      {
        isAllocated[i] = false;
        occupiedSpacesSet.erase(parti._savedLampIndex);
      }
    }
  }

  void show(const Color& color, const uint16_t maxColorIndex, LedStrip& strip)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      // do not show non allocated particles
      if (not isAllocated[i])
        continue;

      const auto& index = particules[i]._savedLampIndex;
      if (is_led_index_valid(index))
        strip.setPixelColor(index, color.get_color(i % maxColorIndex, maxColorIndex));
    }
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
    particules[index] = Particle(to_lamp_unconstraint(pos));
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

#endif

#endif
