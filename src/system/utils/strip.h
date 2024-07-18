#ifndef STRIP_H
#define STRIP_H

#include <Adafruit_NeoPixel.h>

#include <cstdint>
#include <cstring>

#include "../../user_constants.h"
#include "../ext/scale8.h"
#include "constants.h"
#include "coordinates.h"
#include "utils.h"

static constexpr float baseCurrentConsumption =
    0.40;  // strip consumption when all is turned of
static constexpr float maxCurrentConsumption = 2.7 - baseCurrentConsumption;
static constexpr float ampPerLed = maxCurrentConsumption / (float)LED_COUNT;

class LedStrip : public Adafruit_NeoPixel {
 public:
  LedStrip(int16_t pin, neoPixelType type = NEO_RGB + NEO_KHZ800)
      : Adafruit_NeoPixel(LED_COUNT, pin, type) {
    COLOR c;
    c.color = 0;
    for (uint16_t i = 0; i < LED_COUNT; ++i) {
      _colors[i] = c;

      lampCoordinates[i] = to_lamp(i);
    }
  }

  void show() {
    if (hasSomeChanges) {
      // only show if some changes were made
      Adafruit_NeoPixel::show();
    }
    hasSomeChanges = false;
  }

  float estimateCurrentDraw() {
    float estimatedCurrentDraw = 0.0;

    const uint8_t b = getBrightness();
    const float currentPerLed = utils::map(b, 0, 255, 0, ampPerLed);

    for (uint16_t i = 0; i < LED_COUNT; ++i) {
      COLOR c;
      c.color = getRawPixelColor(i);

      const float res = max(c.blue, max(c.red, c.green));
      if (res <= 0) {
        continue;
      }

      estimatedCurrentDraw += currentPerLed;
    }
    return max(baseCurrentConsumption, estimatedCurrentDraw);
  }

  void setPixelColor(uint16_t n, COLOR c) {
    n = constrain(n, 0, LED_COUNT - 1);
    _colors[n] = c;
    hasSomeChanges = true;

    Adafruit_NeoPixel::setPixelColor(n, c.color);
  }

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
    COLOR c;
    c.red = r;
    c.green = g;
    c.blue = b;

    setPixelColor(n, c);
  }

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    COLOR c;
    c.red = r;
    c.green = g;
    c.blue = b;
    c.white = w;

    setPixelColor(n, c);
  }

  void setPixelColor(uint16_t n, uint32_t c) {
    COLOR co;
    co.color = c;

    setPixelColor(n, co);
  }

  void setPixelColorXY(uint16_t x, uint16_t y, COLOR c) {
    setPixelColor(to_strip(x, y), c);
  }

  void setPixelColorXY(uint16_t x, uint16_t y, uint32_t c) {
    setPixelColor(to_strip(x, y), c);
  }

  void fadeToBlackBy(const uint8_t fadeBy) {
    if (fadeBy == 0) return;  // optimization - no scaling to apply

    for (uint16_t i = 0; i < LED_COUNT; ++i) {
      COLOR c;
      c.color = getPixelColor(i);
      setPixelColor(i, utils::color_fade(c, 255 - fadeBy));
    }
  }

  /*
   * Put a value 0 to 255 in to get a color value.
   * The colours are a transition r -> g -> b -> back to r
   * Inspired by the Adafruit examples.
   */
  uint32_t color_wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
      return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) |
             (pos * 3);
    } else if (pos < 170) {
      pos -= 85;
      return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) |
             (255 - pos * 3);
    } else {
      pos -= 170;
      return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) |
             (0);
    }
  }

  void blur(uint8_t blur_amount) {
    if (blur_amount == 0) return;  // optimization: 0 means "don't blur"
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    COLOR carryover;
    carryover.color = 0;
    for (unsigned i = 0; i < LED_COUNT; i++) {
      COLOR cur;
      cur.color = getPixelColor(i);
      COLOR c = cur;
      COLOR part = utils::color_fade(c, seep);
      cur = utils::color_add(utils::color_fade(c, keep), carryover, true);
      if (i > 0) {
        c.color = getPixelColor(i - 1);
        setPixelColor(i - 1, utils::color_add(c, part, true));
      }
      setPixelColor(i, cur);
      carryover = part;
    }
  }

  uint32_t getPixelColor(uint16_t n) const {
    return _colors[constrain(n, 0, LED_COUNT - 1)].color;
  }
  uint32_t getPixelColorXY(int16_t x, int16_t y) const {
    return _colors[constrain(to_strip(x, y), 0, LED_COUNT - 1)].color;
  }

  // Blends the specified color with the existing pixel color.
  void blendPixelColor(uint16_t n, uint32_t color, uint8_t blend) {
    COLOR c1;
    c1.color = getPixelColor(n);
    COLOR c2;
    c2.color = color;
    setPixelColor(n, utils::color_blend(c1, c2, blend));
  }

  // Adds the specified color with the existing pixel color perserving color
  // balance.
  void addPixelColor(uint16_t n, uint32_t color, bool fast = false) {
    COLOR c1;
    c1.color = getPixelColor(n);
    COLOR c2;
    c2.color = color;

    setPixelColor(n, utils::color_add(c1, c2, fast));
  }

  void addPixelColorXY(uint16_t x, uint16_t y, uint32_t color,
                       bool fast = false) {
    addPixelColor(to_strip(x, y), color, fast);
  }

  uint32_t getRawPixelColor(uint16_t n) const {
    return Adafruit_NeoPixel::getPixelColor(n);
  }

  void clear() {
    COLOR c;
    c.color = 0;
    for (uint16_t i = 0; i < LED_COUNT; ++i) {
      _colors[i] = c;
    };
    hasSomeChanges = true;
    Adafruit_NeoPixel::clear();
  }

  inline Cartesian get_lamp_coordinates(uint16_t n) const {
    return lampCoordinates[constrain(n, 0, LED_COUNT - 1)];
  }

  uint32_t* get_buffer_ptr(const uint8_t index) { return _buffers[index]; }

  void buffer_current_colors(const uint8_t index) {
    memcpy(_buffers[index], _colors, sizeof(_colors));
  }

  void fill_buffer(const uint8_t index, const uint32_t value) {
    memset(_buffers[index], value, sizeof(_buffers[index]));
  }

  uint8_t _buffer8b[LED_COUNT];
  uint16_t _buffer16b[LED_COUNT];

 private:
  COLOR _colors[LED_COUNT];
  // buffers for computations
  uint32_t _buffers[2][LED_COUNT];

  // save the expensive computation on world coordinates
  Cartesian lampCoordinates[LED_COUNT];

 private:
  bool hasSomeChanges;
};

#endif