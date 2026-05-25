/**
 * \file strip_impl.h
 * \brief Implement a led strip object
 */

// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_STRIPIMPL_H
#define PLATFORM_STRIPIMPL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include "src/user/constants.h"

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

  LampdaStrip() : begun(false), endTime(0) {};

  void setup(int16_t p, neoPixelType type = NEO_RGB + NEO_KHZ800)
  {
    begun = false;
    endTime = 0;
    updateType(type);
    setPin(p);
  }

  bool begin(void);
  void show(void);

  void setPixelColor(uint16_t n, uint32_t c);
  uint32_t getPixelColor(uint16_t n) const;

  bool canShow(void);

  /**
   * \brief Callback on data written handle
   * \warning DO NOT USE
   */
  void ___irq_dma_update(const uint8_t pwmId, void* pwm, const bool forceUpdate = false);

protected:
  void updateType(neoPixelType t);
  void setPin(int16_t p);

  void write_data_buffers(const uint16_t startIndex, uint16_t* dataBuffer);

private:
  static constexpr uint16_t numBytes = LedCount * ChannelCount;

  static constexpr uint8_t _ledPerBufferCount =
          8; ///< number of leds to display per run, higher is more memory but less CPU
  static constexpr uint16_t _ledChannelPerBufferCount =
          _ledPerBufferCount * ChannelCount;                                 ///< number of bytes to use per led
  static constexpr uint16_t _dataBufferSize = _ledChannelPerBufferCount * 8; ///< size of a WS2812 memory buffer
  static constexpr size_t _expectedBitWritten = numBytes * 8 + 2;            ///< min number of bytes to write

  uint8_t _selectedPwmInterface = 0;  ///< indicates the PWM interface we are using currently
  uint8_t _bufferToWriteIndex = 0;    ///< actual buffer id being written
  bool _isFinished = true;            ///< indicates if the byte streaming is over
  uint16_t _writtenByteCnt = 0;       ///< keep track of the number of byte written
  uint16_t _writtenLedChannelCnt = 0; ///< keep track of the number of led per channel written

  size_t _evt_cnt = 0;
  size_t _call_cnt = 0;

  // 8 bits per leds, 2 buffers
  // THIS MUST NOT BE STATIC AS THE RAM LAYOUT ADRESS IS CRITICAL IN DMA
  uint16_t buffer0[_dataBufferSize];
  uint16_t buffer1[_dataBufferSize];
  std::array<uint16_t*, 2> _dataBuffers = {buffer0, buffer1}; ///< data buffers

private:
  bool begun; ///< true if begin() previously called successfully

  int16_t pin; ///< Output pin number (-1 if not yet set)

  uint8_t rOffset; ///< Red index within each 3- or 4-byte pixel
  uint8_t gOffset; ///< Index of green byte
  uint8_t bOffset; ///< Index of blue byte
  uint8_t wOffset; ///< Index of white (==rOffset if no white)

  uint32_t endTime; ///< Latch timing reference

  /// Store the raw pixels values
  std::array<uint8_t, numBytes> pixels;
};

#ifdef LMBD_LAMP_TYPE__INDEXABLE

extern LampdaStrip<::lampda::LED_COUNT, 3> stripHardwareObject;

#endif

} // namespace strip
} // namespace platform
} // namespace lampda

#endif
