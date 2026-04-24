/*! \file strip.h
    \brief Interface of the indexable strip object. Only used for RGB lamp type
*/

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

#include "src/system/ext/random8.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/vector_math.h"

#include "src/user/constants.h"

namespace lampda {

static constexpr size_t stripNbBuffers = 3;
static constexpr float baseCurrentConsumption = 0.4f;
static constexpr float maxCurrentConsumption = 2.7f - baseCurrentConsumption;
static constexpr float ampPerLed = maxCurrentConsumption / (float)LED_COUNT;

namespace modes::hardware {
struct LampTy;
}

namespace physical {

/// protected inheritence to avoid uncontroled hardware calls
class LedStrip : private Adafruit_NeoPixel
{
  using BufferTy = std::array<uint32_t, LED_COUNT>;
  friend struct modes::hardware::LampTy;

  // need AT LEAST 2*30 FPS for a smooth animation
  static constexpr bool useTemporalDithering = MAIN_LOOP_UPDATE_PERIOD_MS < static_cast<uint32_t>(1000 / 60.0f);
  static constexpr bool randomizeErrorDistributionsInTemporalDithering = true;

public:
  LedStrip(int16_t pin, neoPixelType type = NEO_RGB + NEO_KHZ800) : Adafruit_NeoPixel(LED_COUNT, pin, type)
  {
    COLOR c;
    c.color = 0;
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      if constexpr (useTemporalDithering)
      {
        COLOR initialError;
        _colorErrors[i] = initialError;
      }

      _colors[i] = c;
    }
  }

  void show()
  {
    if (hasSomeChanges)
    {
      // only show if some changes were made
      show_now();
    }
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

  /// Brightness is an internal counter
  void setBrightness(uint8_t b) { brightness = b; }

  /// Brightness is an internal flag
  uint8_t getBrightness() const
  {
    // check that this never changes
    assert(Adafruit_NeoPixel::getBrightness() == 255);

    return brightness;
  }

  // Accessor to set a color
  void setPixelColor(uint16_t n, COLOR c)
  {
    if (n >= LED_COUNT)
      return;

    _colors[n] = c;
  }

  /**
   * EXPLICIT CALLS TO THE LIBRARY
   */

  void begin()
  {
    Adafruit_NeoPixel::begin();
    // brightness always at the maximum level
    Adafruit_NeoPixel::setBrightness(255);
  }

  /// Return the raw color value stored in the send buffer.
  /// \warning Should not be used except for debug
  uint32_t getRawPixelColor(uint16_t n) const
  {
    // ,o colors outside of strip
    if (n >= LED_COUNT)
      return 0;

    COLOR c;
    c.color = Adafruit_NeoPixel::getPixelColor(n);

    // We use brightnessAtShowTime here, or the colors can break when brightness changed
    c.blue = restore_color_with_brightness(c.blue, brightnessAtShowTime);
    c.green = restore_color_with_brightness(c.green, brightnessAtShowTime);
    c.red = restore_color_with_brightness(c.red, brightnessAtShowTime);

    return c.color;
  }

  /// \private: write the buffered colors to the led driver
  void write_to_led_driver(const uint8_t writeBrightness)
  {
    // Adjust brightness to the desired output
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      COLOR& error = _colorErrors[i];
      const COLOR c = convert_color_with_brigthness(_colors[i], writeBrightness, error);
      // set strip color
      Adafruit_NeoPixel::setPixelColor(i, c.color);
    }
  }

  /// Show the current data, independant of changes
  void show_now()
  {
    brightnessAtShowTime = brightness;
    write_to_led_driver(brightnessAtShowTime);

    Adafruit_NeoPixel::show();
    hasSomeChanges = false;
  }

  /**
   * END OF EXPLICIT CALLS
   */

  /**
   * \brief Convert a color to the standard range, assuming it starts as brightness level.
   * \warning This breaks the color resolution, on purpose !
   *\param[in] colorIn Raw color data to convert
   *\param[in] brightness Desired brightness level (0 - 255)
   * \return converted color
   */
  static uint8_t restore_color_with_brightness(uint8_t colorIn, const uint8_t brightness)
  {
    // only special case
    const uint16_t colorShifted = (uint16_t)colorIn << 8;
    if (colorShifted <= brightness or brightness == 0)
      return 0;

    uint8_t fullColor = (colorShifted - brightness) / brightness;
    return fullColor;
  }

  /**
   * \brief Convert a color to the desired brightness level, with an error adjustment
   *\param[in] colorIn Raw color data to convert
   *\param[in] errorIn Additive color error from last run
   *\param[in] brightness Desired brightness level (0 - 255)
   * \return Pair of converted color and new error component
   */
  static std::pair<uint8_t, uint8_t> get_brightness_color_and_error(uint8_t colorIn,
                                                                    uint8_t errorIn,
                                                                    const uint8_t brightness)
  {
    // only special case
    if (colorIn == 0)
      return {0, 0};

    uint16_t fullColor = colorIn * brightness + brightness;
    if ((fullColor >> 8) == 0)
    {
      return {0, 0};
    }
    fullColor += errorIn;
    return {fullColor >> 8, fullColor & 0xFF};
  }

  /**
   * \brief Convert a color to the correct brightness, with temporal dithering.
   * \warning The given brightness should be 0-255
   * \param[in] c Color to convert, in the full scale 0-255, unadjusted to brightness
   * \param[in] brightness The brightness to apply (0 - 255).
   * \param[in, out] error Error components of the current colors
   */
  COLOR convert_color_with_brigthness(const COLOR& c, const uint8_t brightness, COLOR& error)
  {
    // no temporal dithering: no error propagation
    if constexpr (not useTemporalDithering)
    {
      error.red = 0;
      error.green = 0;
      error.blue = 0;
    }
    // convert colors and errors
    const auto& [red, redError] = get_brightness_color_and_error(c.red, error.red, brightness);
    const auto& [green, greenError] = get_brightness_color_and_error(c.green, error.green, brightness);
    const auto& [blue, blueError] = get_brightness_color_and_error(c.blue, error.blue, brightness);

    // store error components
    error.red = redError;
    error.green = greenError;
    error.blue = blueError;

    if constexpr (useTemporalDithering && randomizeErrorDistributionsInTemporalDithering)
    {
      // error scaled
      error.red += (error.red == 0 or error.red == 255 ? 0 : random8() % error.red);
      error.green += (error.green == 0 or error.green == 255 ? 0 : random8() % error.green);
      error.blue += (error.blue == 0 or error.blue == 255 ? 0 : random8() % error.blue);
    }

    COLOR result;
    result.red = red;
    result.green = green;
    result.blue = blue;
    return result;
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

  static uint16_t to_strip(uint16_t screenX, uint16_t screenY)
  {
    if (screenX > stripXCoordinates)
      screenX = stripXCoordinates;
    if (screenY > stripYCoordinates)
      screenY = stripYCoordinates;

    return lmpd_constrain<uint16_t>(screenX + screenY * stripXCoordinates, 0, LED_COUNT - 1);
  }

  void setPixelColorXY(uint16_t x, uint16_t y, COLOR c) { setPixelColor(LedStrip::to_strip(x, y), c); }

  void setPixelColorXY(uint16_t x, uint16_t y, uint32_t c) { setPixelColor(LedStrip::to_strip(x, y), c); }

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
  uint32_t getPixelColorXY(int16_t x, int16_t y) const { return getPixelColor(LedStrip::to_strip(x, y)); }

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
    addPixelColor(LedStrip::to_strip(x, y), color, fast);
  }

  // signal the strip that it can display the update
  void signal_display() { hasSomeChanges = true; }

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

  COLOR _colors[LED_COUNT];
  COLOR _colorErrors[LED_COUNT]; ///< used for temporal dithering

  // buffers for computations
  BufferTy _buffers[stripNbBuffers];

private:
  volatile bool hasSomeChanges;

  /// Out strip brightness
  volatile uint8_t brightness;

  /// Store a reference to the brightness value from the last show() call
  volatile uint8_t brightnessAtShowTime;
};

} // namespace physical
} // namespace lampda

#endif

#endif
