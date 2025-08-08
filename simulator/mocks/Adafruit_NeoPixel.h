#ifndef ADAFRUIT_NEOPIXEL_MOCK_H
#define ADAFRUIT_NEOPIXEL_MOCK_H

#include "src/system/utils/colorspace.h"
#include "src/system/utils/utils.h"

#include <memory>

using neoPixelType = int;

static constexpr int NEO_RGB = 0;
static constexpr int NEO_KHZ800 = 1;

class Adafruit_NeoPixel
{
public:
  Adafruit_NeoPixel(size_t ledCount, int16_t pin, neoPixelType type) { brightness = 255; };

  void begin() {}

  void show() {}

  uint8_t getBrightness() const { return brightness; }
  void setBrightness(uint8_t bright) { brightness = bright; }

  void fill(uint32_t c, int a, int b) {}

  void setPixelColor(uint16_t n, uint32_t c) {}

  uint32_t getPixelColor(uint16_t n) const { return 0; }

  void clear() {};

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return (r << 16) | (g << 8) | b; }

  static uint32_t ColorHSV(double h, double s = 255, double v = 255)
  {
    return utils::ColorSpace::HSV(h, s, v).get_rgb().color;
  }

  // vars
  uint8_t brightness;
};

#endif
