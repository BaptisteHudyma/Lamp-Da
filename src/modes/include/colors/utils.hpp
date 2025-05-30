#ifndef MODES_COLORS_UTILS_HPP
#define MODES_COLORS_UTILS_HPP

/** \file
 *
 *  \brief Manipulate color representations
 */

#include "src/modes/include/compile.hpp"
#include "src/modes/include/colors/palettes.hpp"

//
// Color utilities likely already defined elsewhere
//  (but this time in pure constexpr-able INLINE c++)
//

/// Tools to manipulate colors and their representation
namespace modes::colors {

/// Return color (r, g, b) as a single uint32_t integer
static constexpr LMBD_INLINE uint32_t fromRGB(uint8_t r, uint8_t g, uint8_t b)
{
  uint32_t rx = r;
  uint32_t gx = g;
  return (rx << 16) | (gx << 8) | b;
}

static constexpr uint8_t colorRotation[360] = {
        0,   0,   0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,   9,   11,  12,  13,  15,  17,  18,
        20,  22,  24,  26,  28,  30,  32,  35,  37,  39,  42,  44,  47,  49,  52,  55,  58,  60,  63,  66,  69,  72,
        75,  78,  81,  85,  88,  91,  94,  97,  101, 104, 107, 111, 114, 117, 121, 124, 127, 131, 134, 137, 141, 144,
        147, 150, 154, 157, 160, 163, 167, 170, 173, 176, 179, 182, 185, 188, 191, 194, 197, 200, 202, 205, 208, 210,
        213, 215, 217, 220, 222, 224, 226, 229, 231, 232, 234, 236, 238, 239, 241, 242, 244, 245, 246, 248, 249, 250,
        251, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 251, 250,
        249, 248, 246, 245, 244, 242, 241, 239, 238, 236, 234, 232, 231, 229, 226, 224, 222, 220, 217, 215, 213, 210,
        208, 205, 202, 200, 197, 194, 191, 188, 185, 182, 179, 176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144,
        141, 137, 134, 131, 127, 124, 121, 117, 114, 111, 107, 104, 101, 97,  94,  91,  88,  85,  81,  78,  75,  72,
        69,  66,  63,  60,  58,  55,  52,  49,  47,  44,  42,  39,  37,  35,  32,  30,  28,  26,  24,  22,  20,  18,
        17,  15,  13,  12,  11,  9,   8,   7,   6,   5,   4,   3,   2,   2,   1,   1,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0};

/// Given a 360 degrees angle, return a corresponding color as an integer
static constexpr LMBD_INLINE uint32_t fromAngleHue(uint16_t angleDegrees)
{
  uint8_t r = colorRotation[(angleDegrees + 120) % 360];
  uint8_t g = colorRotation[angleDegrees % 360];
  uint8_t b = colorRotation[(angleDegrees + 240) % 360];
  return fromRGB(r, g, b);
}

/** \brief Given a 0-255 value, return a color approximating a kelvin temperature
 *
 * See modes::colors::PaletteBlackBodyColors
 */
static constexpr LMBD_INLINE uint32_t fromTemp(uint8_t temp)
{
  return from_palette<false>(temp, PaletteBlackBodyColors);
}

} // namespace modes::colors

#endif
