/*! \file time_mock.cpp
    \brief Mock of the board time
*/

#include "src/system/platform/time.h"

#define PLATFORM_TIME_CPP

#include <SFML/System/Time.hpp>

#include "simulator_state.h"

#include <stdint.h>

namespace simulator {
// time
static sf::Clock s_clock;

// HACK to start the system on time zero
static bool isClockReset = false;

namespace time_mocks {
void reset() { isClockReset = false; }
} // namespace time_mocks

} // namespace simulator

namespace lampda {
namespace platform {

uint32_t time_ms(void)
{
  if (!simulator::isClockReset)
  {
    simulator::isClockReset = true;
    simulator::s_clock.restart();
  }

  return simulator::s_clock.getElapsedTime().asMilliseconds() * simulator::globals::state.slowTimeFactor;
}

uint64_t time_us(void)
{
  if (!simulator::isClockReset)
  {
    simulator::isClockReset = true;
    simulator::s_clock.restart();
  }
  return simulator::s_clock.getElapsedTime().asMicroseconds() * simulator::globals::state.slowTimeFactor;
}

void delay_ms(uint32_t dwMs) { sf::sleep(sf::milliseconds(dwMs / simulator::globals::state.slowTimeFactor)); }

void delay_us(uint64_t dwUs) { sf::sleep(sf::microseconds(dwUs / simulator::globals::state.slowTimeFactor)); }

} // namespace platform
} // namespace lampda
