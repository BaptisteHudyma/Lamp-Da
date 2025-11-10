#ifndef UTILS_H
#define UTILS_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

#include "constants.h"

#define SQR(x)      ((x) * (x))
#define POW2(x)     SQR(x)
#define POW3(x)     ((x) * (x) * (x))
#define POW4(x)     (POW2(x) * POW2(x))
#define POW7(x)     (POW3(x) * POW3(x) * (x))
#define DegToRad(x) ((x) * c_PI / 180)
#define FADE16(x)   scale16(x, x)
#define FADE8(x)    scale8(x, x)

#ifndef Arduino_h

template<typename N> static constexpr N min(const N a, const N b)
{
#ifdef LMBD_CPP17
  assert(not std::isnan(a) && "invalid param a");
  assert(not std::isnan(b) && "invalid param b");
#endif
  return a < b ? a : b;
}
template<typename N> static constexpr N max(const N a, const N b)
{
#ifdef LMBD_CPP17
  assert(not std::isnan(a) && "invalid param a");
  assert(not std::isnan(b) && "invalid param b");
#endif
  return a > b ? a : b;
}

template<typename N> static constexpr N abs(const N a) { return std::abs(N(a)); }

#endif

template<typename T> static constexpr T lmpd_constrain(const T& a, const T& mini, const T& maxi)
{
#ifdef LMBD_CPP17
  assert(static_cast<float>(mini) < static_cast<float>(maxi) && "invalid parameters");
#endif
  // prevent invalid values
  return std::isnan(a) ? static_cast<T>(mini) :
                         // constrain the value
                 (static_cast<float>(a) <= static_cast<float>(mini)) ? static_cast<T>(mini) :
         (static_cast<float>(a) >= static_cast<float>(maxi))         ? static_cast<T>(maxi) :
                                                                       static_cast<T>(a);
}

template<typename T = float>
static inline T lmpd_map(float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
  using numeric_limits_T = std::numeric_limits<T>;
  assert(not(std::isnan(in_min) or std::isinf(in_min)) && "in_min invalid");
  assert(not(std::isnan(in_max) or std::isinf(in_max)) && "in_max invalid");
  assert(out_min >= numeric_limits_T::lowest() && out_min <= numeric_limits_T::max() && "out_min invalid");
  assert(not(std::isnan(out_min) or std::isinf(out_min)) && "out_min invalid");
  assert(out_max >= numeric_limits_T::lowest() && out_max <= numeric_limits_T::max() && "out_max invalid");
  assert(not(std::isnan(out_max) or std::isinf(out_max)) && "out_max invalid");

  if (std::isnan(x))
    return out_min;
  if (x > 0 && std::isinf(x))
    return out_max;
  if (std::isinf(x))
    return out_min;
  const float res = (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
  if (res > numeric_limits_T::max())
    return numeric_limits_T::max();
  if (res < numeric_limits_T::lowest())
    return numeric_limits_T::lowest();

  return static_cast<T>(res);
}

static constexpr float to_radians(float degrees)
{
#ifdef LMBD_CPP17
  assert(not(std::isnan(degrees) or std::isinf(degrees)) && "invalid value");
#endif
  return degrees * M_PI / 180.f;
}

inline float wrap_angle(const float angle_rad)
{
  if (angle_rad >= 0 and angle_rad < c_TWO_PI)
    return angle_rad;
  return angle_rad - c_TWO_PI * floor(angle_rad / c_TWO_PI);
}

/**
 * \brief Use this to convert color to bytes
 */
union COLOR
{
  uint32_t color;

  struct
  {
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
uint32_t get_random_complementary_color(const uint32_t color, const float tolerance);

/**
 * \brief Return the color gradient between colorStart to colorEnd
 * \param[in] colorStart Start color of the gradient
 * \param[in] colorEnd End color of the gradient
 * \param[in] level between 0 and 1, the gradient between the two colors
 */
uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd, const float level);
COLOR color_blend(COLOR color1, COLOR color2, uint16_t blend, bool b16 = false);

COLOR color_fade(COLOR c1, uint8_t amount, bool video = false);
COLOR color_add(COLOR c1, COLOR c2, bool fast = false);

uint32_t hue_to_rgb_sinus(const uint16_t angle);

/** \brief Hash input string into a 32-bit unsigned integer
 *
 * This is the xor-variant of the "djb2 hash" in its iterative form, starting
 * from the beginning of the string for simplicity.
 *
 * \param[in] s Zero-terminated input string
 * \param[in] maxSize (optional) Maximal byte count to process, defaults to 14
 * \param[in] off (optional) Skip the first \p off bytes, defaults to 0
 * \remark By default, only the first 14 bytes of the string are used!
 */
template<typename T> static constexpr uint32_t hash(const T s, const uint16_t maxSize = 14, const uint16_t off = 0)
{
#ifdef LMBD_CPP17
  uint32_t hashAcc = 5381;
  for (uint16_t I = 0; I < maxSize; ++I)
  {
    if (s[I + off] == '\0')
    {
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
template<int16_t N> static constexpr uint32_t hash(const char (&s)[N])
{
  static_assert((N - 1 <= 14) && "Use hash(s, maxSize) to hash strings longer than 14 bytes!");
  return hash(s, 14);
}

// Convert a read on an analog pin to a voltage value
constexpr double analogReadToVoltage(const uint16_t analogVal)
{
  return analogVal * internalReferenceVoltage / (float)ADC_MAX_VALUE;
}
constexpr uint16_t voltageToAnalogRead(const float voltage)
{
  return lmpd_constrain<uint16_t>(voltage, 0, internalReferenceVoltage) * ADC_MAX_VALUE / internalReferenceVoltage;
}

}; // namespace utils

#endif
