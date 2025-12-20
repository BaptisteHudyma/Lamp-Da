#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "coordinates.h"

#include <cmath>
#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/utils/utils.h"

uint16_t to_strip(uint16_t screenX, uint16_t screenY)
{
  if (screenX > stripXCoordinates)
    screenX = stripXCoordinates;
  if (screenY > stripYCoordinates)
    screenY = stripYCoordinates;

  return lmpd_constrain<uint16_t>(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
}

#endif
