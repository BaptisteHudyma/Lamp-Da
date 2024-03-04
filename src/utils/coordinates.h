#ifndef UTILS_COORDINATES_H
#define UTILS_COORDINATES_H

#include <cstdint>

/**
 * X if the vertical axis, starting at zero and ending at stripXCoordinates
 */
uint16_t to_screen_x(const uint16_t ledIndex);

/**
 * Y if the horizontal axis, starting at zero and ending at stripYCoordinates
 */
uint16_t to_screen_y(const uint16_t ledIndex);

/**
 * Z if the depth axis, zero is the center of the lamp, the value returned by z indicates the surface of the lamp
 */
uint16_t to_screen_z(const uint16_t ledIndex);

/**
 * Given the x and y, return the led index
 */
uint16_t to_strip(const uint16_t x, const uint16_t y);
 


#endif