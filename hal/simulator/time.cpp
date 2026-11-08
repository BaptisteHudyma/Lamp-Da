/*! \file time.cpp
    \brief Time for the simulator
*/

#include "src/system/hal/time.h"

#include <SFML/System/Time.hpp>

#include "simulator_state.h"

#include <stdint.h>

namespace simulator {
// time
static sf::Clock s_clock;

// HACK to start the system on time zero
static bool isClockReset = false;
static uint32_t clockOffset = 0;

namespace time_mocks {
void reset(uint32_t startClock)
{
  clockOffset = startClock;
  isClockReset = false;
}
} // namespace time_mocks

} // namespace simulator

namespace lampda {
namespace hal {

uint32_t time_ms(void)
{
  if (!simulator::isClockReset)
  {
    simulator::isClockReset = true;
    simulator::s_clock.restart();
  }

  // computed in double, then wrapped modulo 2^32 as the hardware clock: a float loses precision near the wrap, and
  // converting a float out of the uint32 range is undefined
  const double time_ms =
          (static_cast<double>(simulator::clockOffset) + simulator::s_clock.getElapsedTime().asMilliseconds()) *
          simulator::globals::state.slowTimeFactor;
  return static_cast<uint32_t>(static_cast<uint64_t>(time_ms));
}

uint64_t time_us(void)
{
  if (!simulator::isClockReset)
  {
    simulator::isClockReset = true;
    simulator::s_clock.restart();
  }
  // in double: the offset in microseconds does not fit in 32 bits
  const double time_us = (static_cast<double>(simulator::clockOffset) * 1000.0 +
                          simulator::s_clock.getElapsedTime().asMicroseconds()) *
                         simulator::globals::state.slowTimeFactor;
  return static_cast<uint64_t>(time_us);
}

void delay_ms(uint32_t dwMs) { sf::sleep(sf::milliseconds(dwMs / simulator::globals::state.slowTimeFactor)); }

void delay_us(uint64_t dwUs) { sf::sleep(sf::microseconds(dwUs / simulator::globals::state.slowTimeFactor)); }

} // namespace hal
} // namespace lampda
