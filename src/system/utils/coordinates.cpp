#include "coordinates.h"

#include <cmath>
#include <cstdint>

#include "../ext/math8.h"
#include "constants.h"

int16_t to_screen_x(const uint16_t ledIndex) {
  if (ledIndex > LED_COUNT) return 0;
  return cos16(ledIndex * 1.0/ledPerTurn * 2 * 3.14159265358 * ceil(ledPerTurn));
}

int16_t to_screen_y(const uint16_t ledIndex) {
  if (ledIndex > LED_COUNT) return 0;
  return sin16(ledIndex * 1.0/ledPerTurn * 2 * 3.14159265358 * ceil(ledPerTurn));
}

int16_t to_screen_z(const uint16_t ledIndex){
  if (ledIndex > LED_COUNT) return 0;
  return (ledIndex * 1.0/LED_COUNT) * INT16_MAX; 

}

uint16_t to_strip(uint16_t screenX, uint16_t screenY) {
  if (screenX > stripXCoordinates) screenX = stripXCoordinates;
  if (screenY > stripYCoordinates) screenY = stripYCoordinates;

  return constrain(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
}

Cartesian to_lamp(const uint16_t ledIndex){
  return Cartesian(to_screen_x(ledIndex), to_screen_y(ledIndex), to_screen_z(ledIndex));
}

