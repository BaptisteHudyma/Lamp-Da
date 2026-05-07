/*! \file Adafruit_NeoPixel.h
    \brief Mock of the Adafruit NeoPixel library
*/

#ifndef ADAFRUIT_NEOPIXEL_MOCK_H
#define ADAFRUIT_NEOPIXEL_MOCK_H

#include "src/system/utils/colorspace.h"
#include "src/system/utils/utils.h"

#include <memory>

using neoPixelType = int;

// RGB NeoPixel permutations; white and red offsets are always same
// Offset:        W          R          G          B
#define NEO_RGB ((0 << 6) | (0 << 4) | (1 << 2) | (2)) ///< Transmit as R,G,B
#define NEO_RBG ((0 << 6) | (0 << 4) | (2 << 2) | (1)) ///< Transmit as R,B,G
#define NEO_GRB ((1 << 6) | (1 << 4) | (0 << 2) | (2)) ///< Transmit as G,R,B
#define NEO_GBR ((2 << 6) | (2 << 4) | (0 << 2) | (1)) ///< Transmit as G,B,R
#define NEO_BRG ((1 << 6) | (1 << 4) | (2 << 2) | (0)) ///< Transmit as B,R,G
#define NEO_BGR ((2 << 6) | (2 << 4) | (1 << 2) | (0)) ///< Transmit as B,G,R

#define NEO_KHZ800 0x0000 ///< 800 KHz data transmission

class Adafruit_NeoPixel
{
public:
  Adafruit_NeoPixel(size_t n, int16_t pin, neoPixelType t) : begun(false), brightness(0), pixels(NULL)
  {
    updateType(t);
    updateLength(n);
  };

  ~Adafruit_NeoPixel() { free(pixels); }

  void begin() { begun = true; }

  void show()
  {
    /// Cannot implement here, simulator handles this
  }

  uint8_t getBrightness() const { return brightness - 1; }

  /// Implementation similar to Adafruit led strip
  void setBrightness(uint8_t b)
  {
    // Stored brightness value is different than what's passed.
    // This simplifies the actual scaling math later, allowing a fast
    // 8x8-bit multiply and taking the MSB. 'brightness' is a uint8_t,
    // adding 1 here may (intentionally) roll over...so 0 = max brightness
    // (color values are interpreted literally; no scaling), 1 = min
    // brightness (off), 255 = just below max brightness.
    uint8_t newBrightness = b + 1;

    if (newBrightness != brightness)
    { // Compare against prior value
      // Brightness has changed -- re-scale existing data in RAM,
      // This process is potentially "lossy," especially when increasing
      // brightness. The tight timing in the WS2811/WS2812 code means there
      // aren't enough free cycles to perform this scaling on the fly as data
      // is issued. So we make a pass through the existing color data in RAM
      // and scale it (subsequent graphics commands also work at this
      // brightness level). If there's a significant step up in brightness,
      // the limited number of steps (quantization) in the old data will be
      // quite visible in the re-scaled version. For a non-destructive
      // change, you'll need to re-render the full strip data. C'est la vie.
      uint8_t c, *ptr = pixels,
                 oldBrightness = brightness - 1; // De-wrap old brightness value
      uint16_t scale;
      if (oldBrightness == 0)
        scale = 0; // Avoid /0
      else if (b == 255)
        scale = 65535 / oldBrightness;
      else
        scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
      for (uint16_t i = 0; i < numBytes; i++)
      {
        c = *ptr;
        *ptr++ = (c * scale) >> 8;
      }
      brightness = newBrightness;
    }
  }

  void setPixelColor(uint16_t n, uint32_t c)
  {
    if (n < numLEDs)
    {
      uint8_t *p, r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
      if (brightness)
      { // See notes in setBrightness()
        r = (r * brightness) >> 8;
        g = (g * brightness) >> 8;
        b = (b * brightness) >> 8;
      }
      if (wOffset == rOffset)
      {
        p = &pixels[n * 3];
      }
      else
      {
        p = &pixels[n * 4];
        uint8_t w = (uint8_t)(c >> 24);
        p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
      }
      p[rOffset] = r;
      p[gOffset] = g;
      p[bOffset] = b;
    }
  }

  uint32_t getPixelColor(uint16_t n) const
  {
    if (n >= numLEDs)
      return 0; // Out of bounds, return no color.

    uint8_t* p;

    if (wOffset == rOffset)
    { // Is RGB-type device
      p = &pixels[n * 3];
      if (brightness)
      {
        // Stored color was decimated by setBrightness(). Returned value
        // attempts to scale back to an approximation of the original 24-bit
        // value used when setting the pixel color, but there will always be
        // some error -- those bits are simply gone. Issue is most
        // pronounced at low brightness levels.
        return (((uint32_t)(p[rOffset] << 8) / brightness) << 16) | (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
               ((uint32_t)(p[bOffset] << 8) / brightness);
      }
      else
      {
        // No brightness adjustment has been made -- return 'raw' color
        return ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) | (uint32_t)p[bOffset];
      }
    }
    else
    { // Is RGBW-type device
      p = &pixels[n * 4];
      if (brightness)
      { // Return scaled color
        return (((uint32_t)(p[wOffset] << 8) / brightness) << 24) | (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
               (((uint32_t)(p[gOffset] << 8) / brightness) << 8) | ((uint32_t)(p[bOffset] << 8) / brightness);
      }
      else
      { // Return raw color
        return ((uint32_t)p[wOffset] << 24) | ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) |
               (uint32_t)p[bOffset];
      }
    }
  }

  // vars
  bool begun;         ///< true if begin() previously called
  uint16_t numLEDs;   ///< Number of RGB LEDs in strip
  uint16_t numBytes;  ///< Size of 'pixels' buffer below
  uint8_t brightness; ///< Strip brightness 0-255 (stored as +1)
  uint8_t* pixels;    ///< Holds LED color values (3 or 4 bytes each)
  uint8_t rOffset;    ///< Red index within each 3- or 4-byte pixel
  uint8_t gOffset;    ///< Index of green byte
  uint8_t bOffset;    ///< Index of blue byte
  uint8_t wOffset;    ///< Index of white (==rOffset if no white)

private:
  void updateType(neoPixelType t)
  {
    bool oldThreeBytesPerPixel = (wOffset == rOffset); // false if RGBW

    wOffset = (t >> 6) & 0b11; // See notes in header file
    rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
    gOffset = (t >> 2) & 0b11;
    bOffset = t & 0b11;

    // If bytes-per-pixel has changed (and pixel data was previously
    // allocated), re-allocate to new size. Will clear any data.
    if (pixels)
    {
      bool newThreeBytesPerPixel = (wOffset == rOffset);
      if (newThreeBytesPerPixel != oldThreeBytesPerPixel)
        updateLength(numLEDs);
    }
  }

  void updateLength(uint16_t n)
  {
    free(pixels); // Free existing data (if any)

    // Allocate new data -- note: ALL PIXELS ARE CLEARED
    numBytes = n * ((wOffset == rOffset) ? 3 : 4);
    if ((pixels = (uint8_t*)malloc(numBytes)))
    {
      memset(pixels, 0, numBytes);
      numLEDs = n;
    }
    else
    {
      numLEDs = numBytes = 0;
    }
  }
};

#endif
