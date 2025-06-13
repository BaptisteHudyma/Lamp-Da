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
  }

  ParticuleSystem(const uint8_t _desiredParticleCount,
                  const std::function<uint16_t(size_t)>& positionGeneratorFuction) :
    particuleCount(min(maxParticuleCount, _desiredParticleCount))
  {
    ParticuleSystem::occupiedSpacesSet.clear();
    init_particules(positionGeneratorFuction);
  }

  void init_particules(const std::function<uint16_t(size_t)>& positionGeneratorFuction)
  {
    ParticuleSystem::occupiedSpacesSet.clear();
    for (size_t i = 0; i < particuleCount; ++i)
    {
      uint16_t pos = positionGeneratorFuction(i);
      int maxTries = 3;
      while (ParticuleSystem::is_position_taken(pos) and maxTries > 0)
      {
        pos = positionGeneratorFuction(i);
        maxTries--;
      }
      // generate start position from user function
      ParticuleSystem::particules[i] = Particulate(to_lamp(pos));
      ParticuleSystem::occupiedSpacesSet.insert(pos);
    }
  }

  // forced to be less than maxParticuleCount
  const uint8_t particuleCount;

  void iterate_no_collisions(const vec3d& accelerationCartesian, const float deltaTime)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      Particulate& p = ParticuleSystem::particules[i];
      const uint16_t ledIndex = p.to_lamp_index();

      // apply force and constrain
      p.apply_acceleration(accelerationCartesian, deltaTime);
      p.constraint_into_lamp_body();

      // update particule position in occupation set
      const uint16_t newLedIndex = p.to_lamp_index();
      if (newLedIndex != ledIndex)
      {
        ParticuleSystem::occupiedSpacesSet.erase(ledIndex);
        ParticuleSystem::occupiedSpacesSet.insert(newLedIndex);
      }
    }
  }

  void iterate_with_collisions(const vec3d& accelerationCartesian, const float deltaTime)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      Particulate& p = ParticuleSystem::particules[i];
      const uint16_t ledIndex = p.to_lamp_index();

      // simulate instead of updating directly
      Particulate newP = p.simulate_after_acceleration(accelerationCartesian, deltaTime);
      newP.constraint_into_lamp_body();

      // update particule position in occupation set
      uint16_t newLedIndex = newP.to_lamp_index();
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

  void show(const Color& color, LedStrip& strip)
  {
    for (size_t i = 0; i < particuleCount; ++i)
    {
      strip.setPixelColor(ParticuleSystem::particules[i].to_lamp_index(), color.get_color(i, particuleCount));
    }
  }

protected:
  static bool is_position_taken(const uint16_t pos)
  {
    return ParticuleSystem::occupiedSpacesSet.find(pos) != ParticuleSystem::occupiedSpacesSet.cend();
  }

private:
  static constexpr uint8_t maxParticuleCount = 255;
  static inline Particulate particules[maxParticuleCount];
  static inline std::set<uint16_t> occupiedSpacesSet; // store the occupied spaces
};

#endif

#endif