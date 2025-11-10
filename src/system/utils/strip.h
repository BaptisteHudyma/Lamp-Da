#ifndef STRIP_H
#define STRIP_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>
#include <cstring>
#include <array>

#ifndef LMBD_SIMULATION
#include <Adafruit_NeoPixel.h>
#else
#include "simulator/mocks/Adafruit_NeoPixel.h"
#endif

#include "src/system/ext/scale8.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/coordinates.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/vector_math.h"

#include "src/user/constants.h"

static constexpr size_t stripNbBuffers = 2;
static constexpr float baseCurrentConsumption = 0.4f;
static constexpr float maxCurrentConsumption = 2.7f - baseCurrentConsumption;
static constexpr float ampPerLed = maxCurrentConsumption / (float)LED_COUNT;

namespace modes::hardware {
struct LampTy;
}

class LedStrip : public Adafruit_NeoPixel
{
  using BufferTy = std::array<uint32_t, LED_COUNT>;
  friend struct modes::hardware::LampTy;

public:
  LedStrip(int16_t pin, neoPixelType type = NEO_RGB + NEO_KHZ800) : Adafruit_NeoPixel(LED_COUNT, pin, type)
  {
    COLOR c;
    c.color = 0;
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      _colors[i] = c;

      lampCoordinates[i] = to_lamp(i);
    }
  }

  void show()
  {
    if (hasSomeChanges)
    {
      // only show if some changes were made
      Adafruit_NeoPixel::show();
    }
    hasSomeChanges = false;
  }

  void show_now()
  {
    Adafruit_NeoPixel::show();
    hasSomeChanges = false;
  }

  float estimateCurrentDraw()
  {
    float estimatedCurrentDraw = 0.0;

    const uint8_t b = getBrightness();
    const float currentPerLed = lmpd_map<float>(b, 0, 255, 0.0f, ampPerLed);

    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      COLOR c;
      c.color = getRawPixelColor(i);

      const float res = max<float>(c.blue, max<float>(c.red, c.green));
      if (res <= 0)
      {
        continue;
      }

      estimatedCurrentDraw += currentPerLed;
    }
    return max<float>(baseCurrentConsumption, estimatedCurrentDraw);
  }

  void setPixelColor(uint16_t n, COLOR c)
  {
    n = lmpd_constrain<uint16_t>(n, 0, LED_COUNT - 1);
    _colors[n] = c;
    Adafruit_NeoPixel::setPixelColor(n, c.color);
  }

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
  {
    COLOR c;
    c.red = r;
    c.green = g;
    c.blue = b;

    setPixelColor(n, c);
  }

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
  {
    COLOR c;
    c.red = r;
    c.green = g;
    c.blue = b;
    c.white = w;

    setPixelColor(n, c);
  }

  void setPixelColor(uint16_t n, uint32_t c)
  {
    COLOR co;
    co.color = c;

    setPixelColor(n, co);
  }

  void setPixelColorXY(uint16_t x, uint16_t y, COLOR c) { setPixelColor(to_strip(x, y), c); }

  void setPixelColorXY(uint16_t x, uint16_t y, uint32_t c) { setPixelColor(to_strip(x, y), c); }

  void fadeToBlackBy(const uint8_t fadeBy)
  {
    if (fadeBy == 0)
      return; // optimization - no scaling to apply

    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
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
  uint32_t color_wheel(uint8_t pos)
  {
    pos = 255 - pos;
    if (pos < 85)
    {
      return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
    }
    else if (pos < 170)
    {
      pos -= 85;
      return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
    }
    else
    {
      pos -= 170;
      return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
    }
  }

  void blur(uint8_t blur_amount)
  {
    if (blur_amount == 0)
      return; // optimization: 0 means "don't blur"
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    COLOR carryover;
    carryover.color = 0;
    for (unsigned i = 0; i < LED_COUNT; i++)
    {
      COLOR cur;
      cur.color = getPixelColor(i);
      COLOR c = cur;
      COLOR part = utils::color_fade(c, seep);
      cur = utils::color_add(utils::color_fade(c, keep), carryover, true);
      if (i > 0)
      {
        c.color = getPixelColor(i - 1);
        setPixelColor(i - 1, utils::color_add(c, part, true));
      }
      setPixelColor(i, cur);
      carryover = part;
    }
  }

  uint32_t getPixelColor(uint16_t n) const { return _colors[lmpd_constrain<uint16_t>(n, 0, LED_COUNT - 1)].color; }
  uint32_t getPixelColorXY(int16_t x, int16_t y) const
  {
    return _colors[lmpd_constrain<uint16_t>(to_strip(x, y), 0, LED_COUNT - 1)].color;
  }

  // Blends the specified color with the existing pixel color.
  void blendPixelColor(uint16_t n, uint32_t color, uint8_t blend)
  {
    COLOR c1;
    c1.color = getPixelColor(n);
    COLOR c2;
    c2.color = color;
    setPixelColor(n, utils::color_blend(c1, c2, blend));
  }

  // Adds the specified color with the existing pixel color perserving color
  // balance.
  void addPixelColor(uint16_t n, uint32_t color, bool fast = false)
  {
    COLOR c1;
    c1.color = getPixelColor(n);
    COLOR c2;
    c2.color = color;

    setPixelColor(n, utils::color_add(c1, c2, fast));
  }

  void addPixelColorXY(uint16_t x, uint16_t y, uint32_t color, bool fast = false)
  {
    addPixelColor(to_strip(x, y), color, fast);
  }

  uint32_t getRawPixelColor(uint16_t n) const { return Adafruit_NeoPixel::getPixelColor(n); }

  void clear()
  {
    COLOR c;
    c.color = 0;
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      _colors[i] = c;
    }
    Adafruit_NeoPixel::clear();
  }

  // signal the strip that it can display the update
  void signal_display() { hasSomeChanges = true; }

  inline vec3d get_lamp_coordinates(const uint16_t n) const
  {
    return lampCoordinates[lmpd_constrain<uint16_t>(n, 0, LED_COUNT - 1)];
  }
  inline uint16_t get_strip_index_from_lamp_cylindrical_coordinates(const float theta, const float z) const
  {
    return to_led_index(theta, z);
  }

  uint32_t* get_buffer_ptr(const uint8_t index) { return _buffers[index].data(); }

  void buffer_current_colors(const uint8_t index)
  {
    static_assert(sizeof(BufferTy) == sizeof(_colors));
    memcpy(_buffers[index].data(), _colors, sizeof(_colors));
  }

  void fill_buffer(const uint8_t index, const uint32_t value)
  {
    memset(_buffers[index].data(), value, sizeof(BufferTy));
  }

private:
  COLOR _colors[LED_COUNT];

  // buffers for computations
  BufferTy _buffers[stripNbBuffers];

  // save the expensive computation on world coordinates
  vec3d lampCoordinates[LED_COUNT];

private:
  volatile bool hasSomeChanges;
};

#endif

#endif
