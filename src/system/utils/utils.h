#ifndef UTILS_H
#define UTILS_H

#include <cstdint>

#include "constants.h"

#define SQR(x) ((x) * (x))
#define POW2(x) SQR(x)
#define POW3(x) ((x) * (x) * (x))
#define POW4(x) (POW2(x) * POW2(x))
#define POW7(x) (POW3(x) * POW3(x) * (x))
#define DegToRad(x) ((x) * M_PI / 180)
#define FADE16(x) scale16(x, x)
#define FADE8(x) scale8(x, x)

/**
 * \brief Use this to convert color to bytes
 */
union COLOR {
  uint32_t color;

  struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t white;
  };
};

namespace utils {

/**
 * \brief Compute a random color
 */
uint32_t get_random_color();

/**
 * \brief Compute the complementary color of the given color
 * \param[in] color The color to find a complement for
 * \return the complementary color
 */
uint32_t get_complementary_color(const uint32_t color);

/**
 * \brief Compute the complementary color of the given color, with a random
 * variation \param[in] color The color to find a complement for \param[in]
 * tolerance between 0 and 1, the variation tolerance. 1 will give a totally
 * random color, 0 will return the base complementary color \return the random
 * complementary color
 */
uint32_t get_random_complementary_color(const uint32_t color,
                                        const float tolerance);

/**
 * \brief Return the color gradient between colorStart to colorEnd
 * \param[in] colorStart Start color of the gradient
 * \param[in] colorEnd End color of the gradient
 * \param[in] level between 0 and 1, the gradient between the two colors
 */
uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd,
                      const float level);
COLOR color_blend(COLOR color1, COLOR color2, uint16_t blend, bool b16 = false);

COLOR color_fade(COLOR c1, uint8_t amount, bool video = false);
COLOR color_add(COLOR c1, COLOR c2, bool fast = false);

uint32_t hue_to_rgb_sinus(const uint16_t angle);

constexpr float map(float x, float in_min, float in_max, float out_min,
                    float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/** \brief Hash input string into a 32-bit unsigned integer
 *
 * This is the xor-variant of the "djb2 hash" in its iterative form, starting
 * from the beginning of the string for simplicity.
 *
 * \param[in] s Zero-terminated input string
 * \param[in] maxSize (optional) Maximal byte count to process, defaults to 12
 * \param[in] off (optional) Skip the first \p off bytes, defaults to 0
 * \remark By default, only the first 12 bytes of the string are used!
 */
template <typename T>
static constexpr uint32_t hash(const T s,
                               const uint16_t maxSize = 12,
                               const uint16_t off = 0) {
#ifdef LMBD_CPP17
  uint32_t hashAcc = 5381;
  for (uint16_t I = 0; I < maxSize; ++I) {
    if (s[I + off] == '\0') {
      return hashAcc;
    }

    hashAcc = ((hashAcc << 5) + hashAcc) ^ s[I + off];
  }
  return hashAcc;
#else
  return !s[off] ? 5381 : (hash(s, maxSize, off + 1) * 33) ^ s[off];
#endif
}

/// \private Same as hash, but takes priority when called with hash("string")
template <int16_t N>
static constexpr uint32_t hash(const char (&s)[N]) {
  static_assert((N-1 <= 12) && "Use hash(s, maxSize) to hash strings longer than 12 bytes!");
  return hash(s, 12);
}

void calcGammaTable(float gamma);
COLOR gamma32(COLOR color);
uint8_t gamma8(uint8_t value);

// Convert a read on an analog pin to a voltage value
// Depends on the set maxConvertedVoltage !!!!
double analogToDividerVoltage(const uint16_t analogVal);

// is the microcontroler powered by vbus
bool is_powered_with_vbus();

};  // namespace utils

#endif
