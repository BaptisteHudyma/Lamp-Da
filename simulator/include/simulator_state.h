#ifndef SIMULATOR_STATE_H
#define SIMULATOR_STATE_H

#include <SFML/Graphics.hpp>

#include "src/modes/include/hardware/lamp_type.hpp"
using _LampTy = modes::hardware::LampTy;

namespace sim {

namespace globals {

struct GlobalSimStateTy
{
  bool paused = false;
  bool verbose = false;
  bool isButtonPressed = false;
  char lastKeyPressed = 0;
  uint8_t tickAndPause = 0;
  uint16_t brightness = 255;
  uint32_t colorBuffer[_LampTy::ledCount] = {};
  uint32_t indicatorColor = 0;
  float slowTimeFactor = 1.0;
};

extern GlobalSimStateTy state;

} // namespace globals

} // namespace sim

#endif
