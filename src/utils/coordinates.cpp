#include "coordinates.h"

#include <cmath>
#include <cstdint>

#include "../ext/math8.h"
#include "constants.h"

uint16_t to_screen_x(const uint16_t ledIndex) {
  if (ledIndex > LED_COUNT) return 0;

  return round(std::fmod(ledIndex, stripXCoordinates));
}

uint16_t to_screen_y(const uint16_t ledIndex) {
  if (ledIndex > LED_COUNT) return 0;
  static constexpr float divider = 1.0 / stripXCoordinates;
  return floor(ledIndex * divider);
}

uint16_t to_screen_z(const uint16_t ledIndex) { return 1; }

uint16_t to_strip(uint16_t screenX, uint16_t screenY) {
  if (screenX > stripXCoordinates) screenX = stripXCoordinates;
  if (screenY > stripYCoordinates) screenY = stripYCoordinates;

  return constrain(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
}

Cartesian to_lamp(const uint16_t ledIndex) {
  // save some processing power
  static constexpr float calc = 1.0 / stripXCoordinates * UINT16_MAX;
  const uint16_t traj = to_screen_x(ledIndex) * calc;

  const int16_t x = cos16(traj);
  const int16_t y = sin16(traj);

  static constexpr float calc2 = (1.0 / stripYCoordinates) * INT16_MAX * 2.0;
  const int16_t z = to_screen_y(ledIndex) * calc2;

  return Cartesian(x, y, z);
}