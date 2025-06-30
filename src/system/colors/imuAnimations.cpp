#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "imuAnimations.h"

#include "src/system/colors/particle_system/particle_system.h"
#include "src/system/physical/imu.h"

namespace animations {

// ALL IMU ANIMATIONS SHARE THIS PARTICLE SYSTEM
// Spawn another if multiple systems should run in parralel
inline static ParticleSystem particuleSystem = ParticleSystem();

int16_t generate_random_particule_position(size_t) { return random16(LED_COUNT); }
int16_t generate_particule_at_top_random_position(size_t)
{
  return -static_cast<float>(random16(2 * stripXCoordinates));
}
int16_t generate_particule_at_bottom_random_position(size_t)
{
  return LED_COUNT + static_cast<float>(random16(2 * stripXCoordinates));
}
int16_t generate_particule_at_extremes(size_t i)
{
  // pair particules spwan at the top, odd at bottom
  return (i % 2 == 0) ? generate_particule_at_top_random_position(i) : generate_particule_at_bottom_random_position(i);
}

void liquid(const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart)
{
  static bool isFirstInit = true;
  static constexpr uint16_t particuleCount = 255;

  const bool shouldreset = restart || isFirstInit;

  const auto& reading = imu::get_filtered_reading(shouldreset);
  if (shouldreset)
  {
    isFirstInit = false;
    particuleSystem.reset();
    particuleSystem.set_max_particle_count(particuleCount);
    particuleSystem.init_particules(generate_random_particule_position);
    return;
  }

  particuleSystem.iterate_with_collisions(reading.accel, loopDeltaTime);

  strip.fadeToBlackBy(255 - persistance);
  particuleSystem.show(color, particuleCount, strip);
}

bool recycle_particules_if_too_far(const Particle& p)
{
  return p.z_mm > stripXCoordinates * 3 or p.z_mm < -(lampHeight + stripXCoordinates * 3);
}

void rain(const uint8_t rainDensity, const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart)
{
  static constexpr float lightRainDropsPerSecond = 1;
  static constexpr float heavyRainDropsPerSecond = 700;
  static constexpr uint16_t particuleCount = 512;

  static bool isFirstInit = true;
  static float rainDropSpawn = 0.0;

  const bool shouldreset = restart || isFirstInit;

  const auto& reading = imu::get_filtered_reading(shouldreset);

  if (shouldreset)
  {
    isFirstInit = false;
    particuleSystem.reset();
    particuleSystem.set_max_particle_count(particuleCount);
    return;
  }

  // multiply by 2 because we rain on both sides of the lamp, half of the drops are not seen
  const float expectedDropPerSecond = rainDensity / 255.0 * heavyRainDropsPerSecond + lightRainDropsPerSecond;
  rainDropSpawn += expectedDropPerSecond * loopDeltaTime;

  if (rainDropSpawn > 1.0)
  {
    // initialize particules in a deffered way, when free spots are available
    particuleSystem.init_deferred_particules(2 * rainDropSpawn, generate_particule_at_extremes);
    rainDropSpawn = 0.0;
  }

  // no collisions between particles, and with no lamp limits
  static constexpr bool shouldKeepInLampBounds = false;
  particuleSystem.iterate_no_collisions(reading.accel, loopDeltaTime, shouldKeepInLampBounds);
  // depop particules that fell too far
  particuleSystem.depop_particules(recycle_particules_if_too_far);

  strip.fadeToBlackBy(255 - persistance);
  // break palette size to display more colors per drops
  particuleSystem.show(color, 5, strip);
}

} // namespace animations

#endif