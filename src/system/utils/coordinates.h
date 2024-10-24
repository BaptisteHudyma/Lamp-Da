#ifndef UTILS_COORDINATES_H
#define UTILS_COORDINATES_H

#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>

struct Cartesian {
  int16_t x;
  int16_t y;
  int16_t z;

  Cartesian(const int16_t x, const int16_t y, const int16_t z)
      : x(x), y(y), z(z) {};

  Cartesian() : x(0), y(0), z(0) {}
};

/**
 * X is the vertical axis, starting at zero and ending at stripXCoordinates
 */
uint16_t to_screen_x(const uint16_t ledIndex);

/**
 * Y is the horizontal axis, starting at zero and ending at stripYCoordinates
 */
uint16_t to_screen_y(const uint16_t ledIndex);

/**
 * Z is the depth axis, fixed to one (screen space)
 */
uint16_t to_screen_z(const uint16_t ledIndex);

/**
 * Given the x and y, return the led index
 */
uint16_t to_strip(const uint16_t screenX, const uint16_t screenY);

/**
 * project to lamp space (3D space centered on the lamp axis)
 */
Cartesian to_lamp(const uint16_t ledIndex);

#endif

#endif