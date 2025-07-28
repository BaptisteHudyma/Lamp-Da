#ifndef SIMULATOR_STATE_H
#define SIMULATOR_STATE_H

#include <SFML/Graphics.hpp>

#include "src/user/constants.h"

namespace sim {

namespace globals {

struct GlobalSimStateTy
{
  bool paused = false;
  bool verbose = false;
  bool isButtonPressed = false;
  char lastKeyPressed = 0;
  uint8_t brightness = 255;
  uint32_t colorBuffer[LED_COUNT] = {};
  uint32_t indicatorColor = 0;
  float slowTimeFactor = 1.0;
};

extern GlobalSimStateTy state;

} // namespace globals

} // namespace sim

#endif
