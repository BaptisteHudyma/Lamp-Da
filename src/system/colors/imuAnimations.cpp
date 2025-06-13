#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "imuAnimations.h"

#include "src/system/colors/particule_system.h"
#include "src/system/physical/imu.h"

namespace animations {

static constexpr size_t particuleCount = 200;
ParticuleSystem particuleSystem(particuleCount);

uint16_t generate_random_particule_position(size_t) { return random16(LED_COUNT); }

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

} // namespace animations

#endif