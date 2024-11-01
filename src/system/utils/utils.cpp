#include "utils.h"

#include <math.h>
#include <stdlib.h>

#include <cstdint>

#include "src/system/ext/math8.h"
#include "src/system/ext/scale8.h"

#include "colorspace.h"

namespace utils {

/*!
    @brief   Convert separate red, green and blue values into a single
             "packed" 32-bit RGB color.
    @param   r  Red brightness, 0 to 255.
    @param   g  Green brightness, 0 to 255.
    @param   b  Blue brightness, 0 to 255.
    @return  32-bit packed RGB value, which can then be assigned to a
             variable for later use or passed to the setPixelColor()
             function. Packed RGB format is predictable, regardless of
             LED strand color order.
  */
static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
/*!
  @brief   Convert separate red, green, blue and white values into a
           single "packed" 32-bit WRGB color.
  @param   r  Red brightness, 0 to 255.
  @param   g  Green brightness, 0 to 255.
  @param   b  Blue brightness, 0 to 255.
  @param   w  White brightness, 0 to 255.
  @return  32-bit packed WRGB value, which can then be assigned to a
           variable for later use or passed to the setPixelColor()
           function. Packed WRGB format is predictable, regardless of
           LED strand color order.
*/
static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t get_random_color() {
  static uint8_t count;

  uint8_t color[3];
  color[count] = random(256);
  uint8_t a0 = random(1);
  uint8_t a1 = ((!a0) + count + 1) % 3;
  a0 = (count + a0 + 1) % 3;
  color[a0] = 255 - color[count];
  color[a1] = 0;
  count += random(15);  // to avoid repeating patterns
  count %= 3;

  union COLOR colorArray;
  colorArray.red = color[0];
  colorArray.green = color[1];
  colorArray.blue = color[2];
  return colorArray.color;
}

uint32_t get_complementary_color(const uint32_t color) {
  COLOR c;
  c.color = color;
  const uint16_t hue = ColorSpace::HSV(c).get_scaled_hue();

  const uint16_t finalHue = (uint32_t)(hue + UINT16_MAX / 2.0) % UINT16_MAX;
  // add a cardan shift to the hue, to opbtain the symetrical color
  return utils::hue_to_rgb_sinus(map(finalHue, 0, UINT16_MAX, 0, 360));
}

uint32_t get_random_complementary_color(const uint32_t color,
                                        const float tolerance) {
  COLOR c;
  c.color = color;
  const float hue = ColorSpace::HSV(c).h;

  // add random offset
  const float comp =
      360.0 * (0.1 + utils::map(rand(), 0, RAND_MAX, -tolerance / 2.0,
                                tolerance / 2.0));

  const uint16_t finalHue =
      fmod(hue + 360.0 / 2.0 + comp, 360.0f);  // add offset to the hue
  return utils::hue_to_rgb_sinus(finalHue);
}

COLOR color_blend(COLOR color1, COLOR color2, uint16_t blend, bool b16) {
  if (blend == 0) return color1;
  uint16_t blendmax = b16 ? 0xFFFF : 0xFF;
  if (blend == blendmax) return color2;
  uint8_t shift = b16 ? 16 : 8;

  COLOR res;
  res.white =
      ((color2.white * blend) + (color1.white * (blendmax - blend))) >> shift;
  res.red = ((color2.red * blend) + (color1.red * (blendmax - blend))) >> shift;
  res.green =
      ((color2.green * blend) + (color1.green * (blendmax - blend))) >> shift;
  res.blue =
      ((color2.blue * blend) + (color1.blue * (blendmax - blend))) >> shift;

  return res;
}

uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd,
                      const float level) {
  union COLOR colorStartArray, colorEndArray;
  colorStartArray.color = colorStart;
  colorEndArray.color = colorEnd;

  return Color(
      colorStartArray.red + level * (colorEndArray.red - colorStartArray.red),
      colorStartArray.green +
          level * (colorEndArray.green - colorStartArray.green),
      colorStartArray.blue +
          level * (colorEndArray.blue - colorStartArray.blue));
}

/*
 * fades color toward black
 * if using "video" method the resulting color will never become black unless it
 * is already black
 */
COLOR color_fade(COLOR c1, uint8_t amount, bool video) {
  if (video) {
    c1.red = scale8_video(c1.red, amount);
    c1.green = scale8_video(c1.green, amount);
    c1.blue = scale8_video(c1.blue, amount);
    c1.white = scale8_video(c1.white, amount);
  } else {
    c1.red = scale8(c1.red, amount);
    c1.green = scale8(c1.green, amount);
    c1.blue = scale8(c1.blue, amount);
    c1.white = scale8(c1.white, amount);
  }
  return c1;
}

COLOR color_add(COLOR c1, COLOR c2, bool fast) {
  if (fast) {
    c1.red = qadd8(c1.red, c2.red);
    c1.green = qadd8(c1.green, c2.green);
    c1.blue = qadd8(c1.blue, c2.blue);
    c1.white = qadd8(c1.white, c2.white);
    return c1;
  } else {
    uint32_t r = c1.red + c2.red;
    uint32_t g = c1.green + c2.green;
    uint32_t b = c1.blue + c2.blue;
    uint32_t w = c1.white + c2.white;
    uint16_t max = r;
    if (g > max) max = g;
    if (b > max) max = b;
    if (w > max) max = w;
    if (max < 256) {
      c1.red = r;
      c1.green = g;
      c1.blue = b;
      c1.white = w;
      return c1;
    } else {
      c1.red = r * 255 / max;
      c1.green = g * 255 / max;
      c1.blue = b * 255 / max;
      c1.white = w * 255 / max;
      return c1;
    }
  }
}

uint32_t hue_to_rgb_sinus(const uint16_t angle) {
  static const uint8_t lights[360] = {
      0,   0,   0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,
      9,   11,  12,  13,  15,  17,  18,  20,  22,  24,  26,  28,  30,  32,  35,
      37,  39,  42,  44,  47,  49,  52,  55,  58,  60,  63,  66,  69,  72,  75,
      78,  81,  85,  88,  91,  94,  97,  101, 104, 107, 111, 114, 117, 121, 124,
      127, 131, 134, 137, 141, 144, 147, 150, 154, 157, 160, 163, 167, 170, 173,
      176, 179, 182, 185, 188, 191, 194, 197, 200, 202, 205, 208, 210, 213, 215,
      217, 220, 222, 224, 226, 229, 231, 232, 234, 236, 238, 239, 241, 242, 244,
      245, 246, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 255, 255, 255,
      255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 251, 250, 249, 248, 246,
      245, 244, 242, 241, 239, 238, 236, 234, 232, 231, 229, 226, 224, 222, 220,
      217, 215, 213, 210, 208, 205, 202, 200, 197, 194, 191, 188, 185, 182, 179,
      176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144, 141, 137, 134, 131,
      127, 124, 121, 117, 114, 111, 107, 104, 101, 97,  94,  91,  88,  85,  81,
      78,  75,  72,  69,  66,  63,  60,  58,  55,  52,  49,  47,  44,  42,  39,
      37,  35,  32,  30,  28,  26,  24,  22,  20,  18,  17,  15,  13,  12,  11,
      9,   8,   7,   6,   5,   4,   3,   2,   2,   1,   1,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

  union COLOR colorArray;
  colorArray.red = lights[(angle + 120) % 360];
  colorArray.green = lights[angle % 360];
  colorArray.blue = lights[(angle + 240) % 360];
  return colorArray.color;
}

// gamma 2.8 lookup table used for color correction
static uint8_t gammaT[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
    14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,
    31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,
    58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,
    75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
    96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119,
    120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146,
    148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177,
    180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252,
    255};

// re-calculates & fills gamma table
void calcGammaTable(float gamma) {
  for (size_t i = 0; i < 256; i++) {
    gammaT[i] = (int)(powf((float)i / 255.0f, gamma) * 255.0f + 0.5f);
  }
}

uint8_t gamma8(uint8_t value) { return gammaT[value]; }

// used for color gamma correction
COLOR gamma32(COLOR color) {
  color.white = gammaT[color.white];
  color.red = gammaT[color.red];
  color.green = gammaT[color.green];
  color.blue = gammaT[color.blue];
  return color;
}

double analogToDividerVoltage(const uint16_t analogVal) {
  static constexpr float multiplier =
      (1.0 / (float)ADC_MAX_VALUE) * maxConvertedVoltage;
  return analogVal * multiplier;
}

};  // namespace utils
