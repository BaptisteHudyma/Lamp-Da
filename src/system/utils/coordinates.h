#ifndef UTILS_COORDINATES_H
#define UTILS_COORDINATES_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>
#include "src/system/utils/vector_math.h"
#include "src/system/utils/constants.h"

/**
 * \brief Given the x and y, return the led index
 */
uint16_t to_strip(const uint16_t screenX, const uint16_t screenY);

#endif

#endif
