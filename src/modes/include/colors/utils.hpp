#ifndef MODES_COLORS_UTILS_HPP
#define MODES_COLORS_UTILS_HPP

/** \file
 *
 *  \brief Manipulate color representations
 */

#include "src/modes/include/compile.hpp"

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

/// Exposes (r, g, b) as uint8_t in struct from a single uint32_t color
struct ToRGB
{
  constexpr LMBD_INLINE ToRGB(uint32_t color) :
    r {(uint8_t)((color >> 16) & 0xff)},
    g {(uint8_t)((color >> 8) & 0xff)},
    b {(uint8_t)(color & 0xff)}
  {
  }

  uint8_t r, g, b;
};

/// Return color (w, w, w) as a single uint32_t integer
static constexpr LMBD_INLINE uint32_t fromGrey(uint32_t w) { return fromRGB(w, w, w); }

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

} // namespace modes::colors

#include "src/modes/include/colors/palettes.hpp"

namespace modes::colors {

/** \brief Given a 0-255 value, return a color approximating a kelvin temperature
 *
 * See modes::colors::PaletteBlackBodyColors
 */
static constexpr LMBD_INLINE uint32_t fromTemp(uint8_t temp)
{
  return from_palette<false>(temp, PaletteBlackBodyColors);
}

/**
 * \brief blend to two colors
 *
 * @param[in] leftColor    : the first color to use
 * @param[in] rightColor   : the second color to use
 * @param[in] blend        : the amount of the first color use in the blend
 * @param[in] b16 optional : use b16 for a more
 *
 * @return the new color computed
 */
uint32_t blend(uint32_t leftColor, uint32_t rightColor, uint16_t blend, bool b16 = false)
{
  if (blend == 0)
    return leftColor;
  uint16_t blendmax = b16 ? 0xFFFF : 0xFF;
  if (blend >= blendmax)
    return rightColor;
  uint8_t shift = b16 ? 16 : 8;

  ToRGB left_rgb(leftColor);
  ToRGB right_rgb(rightColor);

  return fromRGB(((right_rgb.r * blend) + (left_rgb.r * (blendmax - blend))) >> shift,
                 ((right_rgb.g * blend) + (left_rgb.g * (blendmax - blend))) >> shift,
                 ((right_rgb.b * blend) + (left_rgb.b * (blendmax - blend))) >> shift);
}

/**
 * \brief fade the color toward black
 *
 * if using template "isVideoMode" method the resulting color will never become black unless it
 * is already black
 *
 * @param[in] inputColor   : the first color to use
 * @param[in] fadeAmount   : the second color to use
 *
 * @return the new color computed
 */
template<bool isVideoMode = false> uint32_t fade(uint32_t inputColor, uint8_t fadeAmount)
{
  uint32_t res;
  ToRGB input_rgb(inputColor);

  if (isVideoMode)
  {
    res = fromRGB(scale8_video(input_rgb.r, fadeAmount),
                  scale8_video(input_rgb.r, fadeAmount),
                  scale8_video(input_rgb.b, fadeAmount));
  }
  else
  {
    res = fromRGB(scale8(input_rgb.r, fadeAmount), scale8(input_rgb.r, fadeAmount), scale8(input_rgb.b, fadeAmount));
  }
  return res;
}

} // namespace modes::colors

#endif
