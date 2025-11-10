#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "coordinates.h"

#include <cmath>
#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/utils/utils.h"

float to_helix_x(const int16_t ledIndex) { return stripXCoordinates * cos(ledIndex / ledPerTurn * c_TWO_PI); }

float to_helix_y(const int16_t ledIndex) { return stripXCoordinates * sin(ledIndex / ledPerTurn * c_TWO_PI); }

// the minus is for inverse helix
float to_helix_z(const int16_t ledIndex) { return -ledStripWidth_mm * ledIndex / ledPerTurn; }

uint16_t to_strip(uint16_t screenX, uint16_t screenY)
{
  if (screenX > stripXCoordinates)
    screenX = stripXCoordinates;
  if (screenY > stripYCoordinates)
    screenY = stripYCoordinates;

  return lmpd_constrain<uint16_t>(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
}

vec3d to_lamp(const uint16_t ledIndex)
{
  if (ledIndex > LED_COUNT)
    return vec3d(0, 0, 0);
  return vec3d(to_helix_x(ledIndex), to_helix_y(ledIndex), to_helix_z(ledIndex));
}

vec3d to_lamp_unconstraint(const int16_t ledIndex)
{
  return vec3d(to_helix_x(ledIndex), to_helix_y(ledIndex), to_helix_z(ledIndex));
}

uint16_t to_led_index(const float angle_rad, const float z)
{
  static const uint16_t maxZCoordinate = floor(-to_helix_z(LED_COUNT) / ledStripWidth_mm);

  // snip Z per possible lines
  uint16_t zIndex = floor(-z / ledStripWidth_mm);
  if (zIndex < 0.0)
    zIndex = 0.0;
  if (zIndex > maxZCoordinate)
    zIndex = maxZCoordinate;

  // indexing around the led turn
  const float angularPosition = wrap_angle(angle_rad) / c_TWO_PI * stripXCoordinates;

  // convert to led index (approx)
  int16_t ledIndex = round(angularPosition + zIndex * stripXCoordinates);
  if (ledIndex < 0)
    ledIndex = round(angularPosition + (zIndex + 1) * stripXCoordinates);
  if (ledIndex >= LED_COUNT)
    ledIndex = round(angularPosition + (zIndex - 1) * stripXCoordinates);

  if (ledIndex < 0)
    return 0;
  if (ledIndex >= LED_COUNT)
    return LED_COUNT - 1;
  return ledIndex;
}

int16_t to_led_index_no_bounds(const float angle_rad, const float z)
{
  // snip Z per possible lines
  const int16_t zIndex = floor(-z / ledStripWidth_mm);
  // indexing around the led turn
  const float angularPosition = wrap_angle(angle_rad) / c_TWO_PI * stripXCoordinates;

  // convert to led index (approx)
  return round(angularPosition + zIndex * stripXCoordinates);
}

bool is_lamp_coordinate_out_of_bounds(const float angle_rad, const float z)
{
  return not is_led_index_valid(to_led_index_no_bounds(angle_rad, z));
}

#endif
