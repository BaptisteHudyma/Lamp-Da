#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "coordinates.h"

#include <cmath>
#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

float to_helix_x(const uint16_t ledIndex) { return stripXCoordinates * cos(ledIndex / ledPerTurn * c_TWO_PI); }

float to_helix_y(const uint16_t ledIndex) { return stripXCoordinates * sin(ledIndex / ledPerTurn * c_TWO_PI); }

// the minus is for inverse helix
float to_helix_z(const uint16_t ledIndex) { return -ledStripWidth_mm * ledIndex / ledPerTurn; }

uint16_t to_strip(uint16_t screenX, uint16_t screenY)
{
  if (screenX > stripXCoordinates)
    screenX = stripXCoordinates;
  if (screenY > stripYCoordinates)
    screenY = stripYCoordinates;

  return lmpd_constrain(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
}

vec3d to_lamp(const uint16_t ledIndex)
{
  if (ledIndex > LED_COUNT)
    return vec3d(0, 0, 0);
  return vec3d(to_helix_x(ledIndex), to_helix_y(ledIndex), to_helix_z(ledIndex));
}

uint16_t to_led_index(const vec3d& coordinates)
{
  // snip Z per possible lines
  const int16_t adjustedZ = floor(-coordinates.z / ledStripWidth_mm);

  // convert position to angle around z axis
  float angle = std::atan2(coordinates.y, coordinates.x);
  if (angle < 0)
    angle += c_TWO_PI;
  // indexing around the led turn
  const float position = angle / c_TWO_PI * stripXCoordinates;

  // convert to led index (approx)
  int16_t ledIndex = round(adjustedZ * stripXCoordinates + position * stripXCoordinates);
  if (ledIndex < 0)
    return 0;
  if (ledIndex > LED_COUNT)
    return LED_COUNT;
  return ledIndex;
}

#endif
