#ifndef UTILS_COORDINATES_H
#define UTILS_COORDINATES_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>
#include "src/system/utils/vector_math.h"

/**
 * \brief X is the vertical axis, starting at zero and ending at stripXCoordinates
 */
float to_helix_x(const uint16_t ledIndex);

/**
 * \brief Y is the horizontal axis, starting at zero and ending at stripYCoordinates
 */
float to_helix_y(const uint16_t ledIndex);

/**
 * \brief Z is the depth axis, fixed to one (screen space)
 */
float to_helix_z(const uint16_t ledIndex);

/**
 * \brief Given the x and y, return the led index
 */
uint16_t to_strip(const uint16_t screenX, const uint16_t screenY);

/**
 * \brief project to lamp space (3D helix  centered on the lamp axis)
 */
vec3d to_lamp(const uint16_t ledIndex);

bool is_lamp_coordinate_out_of_bounds(const float angle_rad, const float z);

/**
 * \brief convert a lamp coordinate to a led index
 */
uint16_t to_led_index(const float angle_rad, const float z);

#endif

#endif
