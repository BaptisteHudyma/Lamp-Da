/**
 * \file strip_impl.h
 * \brief Implement a led strip object
 */

// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_STRIPIMPL_H
#define PLATFORM_STRIPIMPL_H

#include <cstddef>
#include <cstdint>

#define NEO_RGB    ((0 << 6) | (0 << 4) | (1 << 2) | (2)) ///< Transmit as R,G,B
#define NEO_RBG    ((0 << 6) | (0 << 4) | (2 << 2) | (1)) ///< Transmit as R,B,G
#define NEO_KHZ800 0x0000                                 ///< 800 KHz data transmission

typedef uint8_t neoPixelType; ///< 3rd arg to Adafruit_NeoPixel constructor

namespace lampda {
namespace platform {
/// Define the interaction layer with an indexable strip
namespace strip {

/**
 * \brief This class is a lightweight port of the Adafruit_Neopixel library, with compile time buffers to have a better
 * memory handling.
 *
 */
template<size_t LedCount, uint8_t ChannelCount> class LampdaStrip
{
  static_assert(ChannelCount == 3 || ChannelCount == 4);

public:
  static constexpr size_t numLEDs = LedCount;
  static constexpr uint8_t channelCount = ChannelCount;

  LampdaStrip(int16_t p, neoPixelType type = NEO_RGB + NEO_KHZ800) : begun(false), endTime(0)
  {
    updateType(type);
    setPin(p);
  }

  bool begin(void);
  void show(void);

  void setPixelColor(uint16_t n, uint32_t c);
  uint32_t getPixelColor(uint16_t n) const;

  bool canShow(void);

protected:
  void updateType(neoPixelType t);
  void setPin(int16_t p);

private:
  bool begun; ///< true if begin() previously called successfully

  int16_t pin; ///< Output pin number (-1 if not yet set)

  uint8_t rOffset; ///< Red index within each 3- or 4-byte pixel
  uint8_t gOffset; ///< Index of green byte
  uint8_t bOffset; ///< Index of blue byte
  uint8_t wOffset; ///< Index of white (==rOffset if no white)

  uint32_t endTime; ///< Latch timing reference

  static constexpr uint16_t numBytes = LedCount * ChannelCount;

  static constexpr uint32_t pattern_size = numBytes * 8 + 2;
  uint16_t pixels_pattern[pattern_size];

  /// Store the raw pixels values
  uint8_t pixels[numBytes];
};

} // namespace strip
} // namespace platform
} // namespace lampda

#endif
