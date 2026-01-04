#include "src/system/platform/time.h"

#define PLATFORM_TIME_CPP

#include <SFML/System/Time.hpp>

#include "simulator_state.h"

// time
static sf::Clock s_clock;

#include <stdint.h>

// HACK to start the system on time zero
static bool isClockReset = false;

namespace time_mocks {
void reset() { isClockReset = false; }
} // namespace time_mocks

uint32_t time_ms(void)
{
  if (!isClockReset)
  {
    isClockReset = true;
    s_clock.restart();
  }

  return s_clock.getElapsedTime().asMilliseconds() * sim::globals::state.slowTimeFactor;
}

uint64_t time_us(void)
{
  if (!isClockReset)
  {
    isClockReset = true;
    s_clock.restart();
  }
  return s_clock.getElapsedTime().asMicroseconds() * sim::globals::state.slowTimeFactor;
}

void delay_ms(uint32_t dwMs) { sf::sleep(sf::milliseconds(dwMs / sim::globals::state.slowTimeFactor)); }

void delay_us(uint64_t dwUs) { sf::sleep(sf::microseconds(dwUs / sim::globals::state.slowTimeFactor)); }
