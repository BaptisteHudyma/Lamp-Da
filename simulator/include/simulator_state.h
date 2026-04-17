/*! \file simulator_state.h
    \brief Contain some simulation state
*/

#ifndef SIMULATOR_STATE_H
#define SIMULATOR_STATE_H

#include <SFML/Graphics.hpp>

#include "src/modes/include/hardware/lamp_type.hpp"

namespace simulator {

using _LampTy = ::lampda::modes::hardware::LampTy;

/// Define the global scope of the simulator
namespace globals {

/// Define the the global simulation state
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

/// Store the global simulation state
extern GlobalSimStateTy state;

} // namespace globals

} // namespace simulator

#endif
