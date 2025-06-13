#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "imuAnimations.h"

#include "src/system/colors/particule_system.h"
#include "src/system/physical/imu.h"

namespace animations {

static constexpr uint8_t particuleCount = 255;
ParticuleSystem particuleSystem(particuleCount);

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

  const auto& reading = imu::get_filtered_reading(restart || isFirstInit);

  static uint32_t lastCall = 0;
  if (restart || isFirstInit)
  {
    isFirstInit = false;
    particuleSystem.init_particules(generate_random_particule_position);
    lastCall = time_ms();
    return;
  }

  const uint32_t newTime = time_ms();
  const float deltaTime = (newTime - lastCall) / 1000.0f;

  particuleSystem.iterate_with_collisions(reading.accel, deltaTime);

  strip.fadeToBlackBy(255 - persistance);
  particuleSystem.show(color, strip);

  lastCall = newTime;
}

bool recycle_particules_if_too_far(const Particulate& p)
{
  return p.z_mm > stripXCoordinates * 3 or p.z_mm < -(lampHeight + stripXCoordinates * 3);
}

static constexpr uint8_t particuleCount2 = 50;
ParticuleSystem particuleSystem2(particuleCount2);

void rain(const uint8_t persistance, const Color& color, LedStrip& strip, const bool restart)
{
  static bool isFirstInit = true;

  const auto& reading = imu::get_filtered_reading(restart || isFirstInit);

  static uint32_t lastCall = 0;
  if (restart || isFirstInit)
  {
    isFirstInit = false;
    lastCall = time_ms();
    return;
  }

  // initialize particules in a deffered way, when free spots are available
  particuleSystem2.init_deferred_particules(4, generate_particule_at_extremes);

  const uint32_t newTime = time_ms();
  const float deltaTime = (newTime - lastCall) / 1000.0f;

  // no collisions between particles, and with no lamp limits
  static constexpr bool shouldKeepInLampBounds = false;
  particuleSystem2.iterate_no_collisions(reading.accel, deltaTime, shouldKeepInLampBounds);
  // depop particules that fell too far
  particuleSystem2.depop_particules(recycle_particules_if_too_far);

  strip.fadeToBlackBy(255 - persistance);
  particuleSystem2.show(color, strip);

  lastCall = newTime;
}

} // namespace animations

#endif