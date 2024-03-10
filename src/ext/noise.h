#ifndef NOISE
#define NOISE

#include <cstdint>

namespace noise8 {

// 1D perlin noise
extern uint8_t inoise(uint16_t x);

// 2D perlin noise
extern uint8_t inoise(uint16_t x, uint16_t y);

// 3D perlin noise
extern uint8_t inoise(uint16_t x, uint16_t y, uint16_t z);

// 1d perlin noise with octaves
extern uint8_t inoise_octaves(uint16_t x, uint8_t octaves, int scale,
                              uint16_t time);

}  // namespace noise8

namespace noise16 {

// 1D perlin noise
extern uint16_t inoise(uint32_t x);

// 2D perlin noise
extern uint16_t inoise(uint32_t x, uint32_t y);

// 3D perlin noise
extern uint16_t inoise(uint32_t x, uint32_t y, uint32_t z);

}  // namespace noise16

#endif