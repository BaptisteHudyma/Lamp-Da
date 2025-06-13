#ifdef LMBD_LAMP_TYPE__INDEXABLE

#ifndef PARTICULE_SYSTEM_H
#define PARTICULE_SYSTEM_H

#include <cstdint>
#include <functional>
#include <set>

#include "src/system/ext/random8.h"
#include "src/system/colors/colors.h"
#include "src/system/colors/particule_cylinder.h"

/**
 * \brief Define a particule system
 * ALL PARTICLE SYSTEM SHARE THE SAME PÄRTICLE SUBSET
 */
class ParticuleSystem
{
public:
  ParticuleSystem() : particuleCount(0) { ParticuleSystem::occupiedSpacesSet.clear(); }

  ParticuleSystem(const uint8_t _desiredParticleCount) : particuleCount(min(maxParticuleCount, _desiredParticleCount))
  {
    ParticuleSystem::occupiedSpacesSet.clear();
    ParticuleSystem::allocatedParticuleIndexes.clear();
  }

  ParticuleSystem(const uint8_t _desiredParticleCount, const std::function<int16_t(size_t)>& positionGeneratorFuction) :
    particuleCount(min(maxParticuleCount, _desiredParticleCount))
  {
    ParticuleSystem::occupiedSpacesSet.clear();
    ParticuleSystem::allocatedParticuleIndexes.clear();
    init_particules(positionGeneratorFuction);
  }

  void init_particules(const std::function<int16_t(size_t)>& positionGeneratorFuction)
  {
    ParticuleSystem::occupiedSpacesSet.clear();
    ParticuleSystem::allocatedParticuleIndexes.clear();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      int16_t pos = positionGeneratorFuction(i);
      int maxTries = 3;
      while (ParticuleSystem::is_position_taken(pos) and maxTries > 0)
      {
        pos = positionGeneratorFuction(i);
        maxTries--;
      }
      // generate start position from user function
      ParticuleSystem::particules[i] = Particulate(to_lamp_unconstraint(pos));
      ParticuleSystem::occupiedSpacesSet.insert(pos);
      ParticuleSystem::allocatedParticuleIndexes.insert(i);
    }
  }

  void init_deferred_particules(uint8_t particlesToPop, const std::function<int16_t(size_t)>& positionGeneratorFuction)
  {
    for (size_t i = 0; i < particuleCount and particlesToPop > 0; ++i)
    {
      if (ParticuleSystem::allocatedParticuleIndexes.find(i) != ParticuleSystem::allocatedParticuleIndexes.cend())
      {
        // already allocated
        continue;
      }
      int16_t pos = positionGeneratorFuction(i);
      int maxTries = 3;
      while (ParticuleSystem::is_position_taken(pos) and maxTries > 0)
      {
        pos = positionGeneratorFuction(i);
        maxTries--;
      }
      // generate start position from user function
      ParticuleSystem::particules[i] = Particulate(to_lamp_unconstraint(pos));
      ParticuleSystem::occupiedSpacesSet.insert(pos);
      ParticuleSystem::allocatedParticuleIndexes.insert(i);
      particlesToPop--;
    }
  }

  // forced to be less than maxParticuleCount
  const uint8_t particuleCount;

  void iterate_no_collisions(const vec3d& accelerationCartesian,
                             const float deltaTime,
                             const bool shouldContrain = true)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      Particulate& p = ParticuleSystem::particules[i];
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
      Particulate& p = ParticuleSystem::particules[i];
      const int16_t ledIndex = p._savedLampIndex;

      // simulate instead of updating directly
      Particulate newP = p.simulate_after_acceleration(accelerationCartesian, deltaTime, shouldContrain);

      // update particule position in occupation set
      const int16_t newLedIndex = newP._savedLampIndex;
      if (newLedIndex != ledIndex)
      {
        // check collision : no collision
        if (not is_position_taken(newLedIndex))
        {
          ParticuleSystem::occupiedSpacesSet.erase(ledIndex);
          ParticuleSystem::occupiedSpacesSet.insert(newLedIndex);
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
   * \brief Filter and respawn particles depending on condition
   */
  void depop_particules(const std::function<bool(const Particulate&)>& shouldDepop)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      auto& parti = ParticuleSystem::particules[i];
      if (shouldDepop(parti))
      {
        ParticuleSystem::allocatedParticuleIndexes.erase(i);
        ParticuleSystem::occupiedSpacesSet.erase(parti._savedLampIndex);
      }
    }
  }

  void show(const Color& color, LedStrip& strip)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      const auto& index = ParticuleSystem::particules[i]._savedLampIndex;
      if (is_led_index_valid(index))
        strip.setPixelColor(index, color.get_color(i, particuleCount));
    }
  }

protected:
  static bool is_position_taken(const int16_t pos)
  {
    return ParticuleSystem::occupiedSpacesSet.find(pos) != ParticuleSystem::occupiedSpacesSet.cend();
  }

private:
  static constexpr uint8_t maxParticuleCount = 255;
  static inline Particulate particules[maxParticuleCount];
  static inline std::set<int16_t> occupiedSpacesSet;         // store the occupied spaces
  static inline std::set<uint8_t> allocatedParticuleIndexes; // store the allcoated particules
};

#endif

#endif