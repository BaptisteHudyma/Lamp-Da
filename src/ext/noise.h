#ifndef NOISE
#define NOISE

#include <cstdint>

namespace noise {

uint8_t inoise8(uint16_t x);

uint8_t inoise8(uint16_t x, uint16_t y);

uint8_t inoise8_octaves(uint16_t x, uint8_t octaves, int scale, uint16_t time);

}


#endif