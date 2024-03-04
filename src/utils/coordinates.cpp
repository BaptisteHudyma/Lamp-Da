#include "coordinates.h"

#include "constants.h"
#include <cstdint>
#include <cmath>

uint16_t to_screen_x(const uint16_t ledIndex)
{
    if(ledIndex > LED_COUNT)
        return 0;

    return round(std::fmod(ledIndex, stripXCoordinates));
}

uint16_t to_screen_y(const uint16_t ledIndex)
{
    if(ledIndex > LED_COUNT)
        return 0;

    return floor(ledIndex / stripXCoordinates);
}

uint16_t to_screen_z(const uint16_t ledIndex)
{
    return 1;
}
 
uint16_t to_strip(const uint16_t x, const uint16_t y)
{
    return min(LED_COUNT -1, x + y * stripXCoordinates);
}