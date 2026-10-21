/*! \file strip.h
    \brief Interface of the indexable strip object. Only used for RGB lamp type
*/

#ifndef COMPONENT_STRIP_H
#define COMPONENT_STRIP_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>
#include <cstring>
#include <array>

#include "src/system/hal/strip_impl.h"

// special case: include the implementation
#include "src/generated/hal/strip_impl.hpp"
//

#include "src/system/utils/colorspace.h"
#include "src/system/utils/constants.h"

#include "src/user/constants.h"

namespace lampda {

static constexpr size_t stripNbBuffers = 3;
static constexpr float baseCurrentConsumption = 0.4f;
static constexpr float maxCurrentConsumption = 2.7f - baseCurrentConsumption;
static constexpr float ampPerLed = maxCurrentConsumption / (float)LED_COUNT;

namespace modes::hardware {
struct LampTy;
}

namespace component {

using StripImpl_t = hal::strip::LampdaStrip<LED_COUNT, 3>;

/// protected inheritence to avoid uncontroled hardware calls
class LedStrip : private StripImpl_t
{
  using BufferTy = std::array<uint32_t, LED_COUNT>;
  friend struct modes::hardware::LampTy;

  /// Use a blue noise pattern to dither the colors
  static constexpr bool useColorDithering = true;
  /// Use time to dither the colors using error diffusion. It needs AT LEAST 2*30 FPS for a smooth animation
  static constexpr bool useTemporalDithering = false;
  static constexpr uint16_t refreshFramesCount = 10; ///< blue noise refresh rate speed

public:
  LedStrip(int16_t pin, neoPixelType type = NEO_RGB + NEO_KHZ800) : StripImpl_t(pin, type), shownCount(0)
  {
    assert(_colorErrors.size() > 0);

    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      if constexpr (useTemporalDithering)
      {
        _colorErrors[i] = utils::ColorSpace::BLACK.get_rgb().color;
      }

      // All to black
      _colors[i] = utils::ColorSpace::BLACK.get_rgb().color;
    }
  }

  LedStrip(const LedStrip&) = delete;
  LedStrip& operator=(LedStrip const&) = delete;

  void show()
  {
    if (hasSomeChanges)
    {
      // only show if some changes were made
      show_now();
    }
    hasSomeChanges = false;
  }

  /// Brightness is an internal counter
  void setBrightness(uint8_t b) { brightness = b; }

  /// Brightness is an internal flag
  uint8_t getBrightness() const { return brightness; }

  /**
   * EXPLICIT CALLS TO THE LIBRARY
   */

  void begin() { StripImpl_t::begin(); }

  /// Return the raw color value stored in the send buffer.
  /// \warning Should not be used except for debug
  uint32_t getRawPixelColor(uint16_t n) const
  {
    // ,o colors outside of strip
    if (n >= LED_COUNT)
      return 0;

    utils::ColorSpace::RGB c(StripImpl_t::getPixelColor(n));
    // We use brightnessAtShowTime here, or the colors can break when brightness changed
    const uint8_t blue = restore_color_with_brightness(c.get_rgb().blue, brightnessAtShowTime);
    const uint8_t green = restore_color_with_brightness(c.get_rgb().green, brightnessAtShowTime);
    const uint8_t red = restore_color_with_brightness(c.get_rgb().red, brightnessAtShowTime);

    return utils::ColorSpace::RGB(red, green, blue).get_rgb().color;
  }

  /// \private: write the buffered colors to the led driver
  void write_to_led_driver(const uint8_t writeBrightness)
  {
    // Adjust brightness to the desired output
    const uint8_t capedShown = shownCount % refreshFramesCount;
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      if constexpr (useTemporalDithering)
      {
        const uint32_t c = convert_color_with_brightness(_colors[i], writeBrightness, i + capedShown, _colorErrors[i]);
        // set strip color
        StripImpl_t::setPixelColor(i, c);
      }
      else
      {
        const uint32_t c = convert_color_with_brightness(_colors[i], writeBrightness, i + capedShown, _colorErrors[0]);
        // set strip color
        StripImpl_t::setPixelColor(i, c);
      }
    }
  }

  /// Show the current data, independant of changes
  void show_now()
  {
    // copy the pattern to show to the display buffer
    brightnessAtShowTime = brightness;
    write_to_led_driver(brightnessAtShowTime);
    // show on hardware
    StripImpl_t::show();
    hasSomeChanges = false;
    // increment show count
    auto refCount = shownCount;
    shownCount = refCount + 1;
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
    const uint16_t colorShifted = static_cast<uint16_t>(colorIn) << 8u;
    if (colorShifted <= brightness or brightness == 0)
      return 0;

    uint8_t fullColor = static_cast<uint8_t>((colorShifted - brightness) / brightness);
    return fullColor;
  }

  /**
   * \brief Convert a color to the desired brightness level, with an error adjustment
   * \param[in] colorIn Raw color data to convert
   * \param[in] errorIn Additive color error from last run
   * \param[in] brightness Desired brightness level (0 - 255)
   * \param[in] index Index of the noise to use
   * \return Pair of converted color and new error component
   */
  static std::pair<uint8_t, uint8_t> get_brightness_color_and_error(const uint8_t colorIn,
                                                                    uint16_t errorIn,
                                                                    const uint8_t brightness,
                                                                    const uint16_t index)
  {
    // blue noise look up table
    static constexpr std::array<uint8_t, 64> BLUE_NOISE_LUT = {
            0,  32, 8,  40, 2,  34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4,  36, 14, 46,
            6,  38, 60, 28, 52, 20, 62, 30, 54, 22, 3,  35, 11, 43, 1,  33, 9,  41, 51, 19, 59, 27,
            49, 17, 57, 25, 15, 47, 7,  39, 13, 45, 5,  37, 63, 31, 55, 23, 61, 29, 53, 21};

    // only special case
    if (colorIn == 0)
      return {0, 0};

    // scale color by brightness
    const uint16_t scaledColor = colorIn * brightness + brightness;

    // When using standard and temporal dithering, shift the pattern
    if constexpr (useColorDithering and useTemporalDithering)
    {
      const uint8_t allowedError = UINT8_MAX - (scaledColor & 0xFF);
      errorIn += min<uint16_t>(allowedError, BLUE_NOISE_LUT[index % BLUE_NOISE_LUT.size()]);
    }

    // add error
    const uint32_t fullColor = min<uint32_t>(scaledColor + errorIn, 0xFF00);

    // compute final color and new error
    uint32_t finalColorRaw = fullColor >> 8;
    const uint8_t finalError = fullColor & 0xFF;

    // When using only color dithering, add a blue noise error to the final color
    if constexpr (useColorDithering and not useTemporalDithering)
    {
      finalColorRaw += (finalError > BLUE_NOISE_LUT[index % BLUE_NOISE_LUT.size()] ? 1 : 0);
    }

    const uint8_t finalColor = (finalColorRaw > 255) ? 255 : static_cast<uint8_t>(finalColorRaw);
    return {finalColor, finalError};
  }

  /**
   * \brief Convert a color to the correct brightness, with temporal dithering.
   * \warning The given brightness should be 0-255
   * \param[in] c Color to convert, in the full scale 0-255, unadjusted to brightness
   * \param[in] brightness The brightness to apply (0 - 255).
   * \param[in] index Index of the noise to use
   * \param[in, out] error Error components of the current colors
   */
  static uint32_t convert_color_with_brightness(const uint32_t color,
                                                const uint8_t brightness,
                                                const uint16_t index,
                                                uint32_t& error)
  {
    // no temporal dithering: no error propagation
    if constexpr (not useTemporalDithering)
    {
      error = 0;
    }

    utils::ColorSpace::RGB c(color);
    utils::ColorSpace::RGB err(error);

    // convert colors and errors, with small offsets for the different chanels
    const auto& [red, redError] = get_brightness_color_and_error(c.get_rgb().red, err.get_rgb().red, brightness, index);
    const auto& [green, greenError] =
            get_brightness_color_and_error(c.get_rgb().green, err.get_rgb().green, brightness, index + 1);
    const auto& [blue, blueError] =
            get_brightness_color_and_error(c.get_rgb().blue, err.get_rgb().blue, brightness, index + 2);

    // store error components
    error = utils::ColorSpace::RGB(redError, greenError, blueError).get_rgb().color;
    return utils::ColorSpace::RGB(red, green, blue).get_rgb().color;
  }

  void setPixelColor(uint16_t n, uint32_t c)
  {
    if (n >= LED_COUNT)
      return;

    _colors[n] = c;
  }

  uint32_t getPixelColor(uint16_t n) const { return _colors[lmpd_constrain<uint16_t>(n, 0, LED_COUNT - 1)]; }

  // signal the strip that it can display the update
  void signal_display() { hasSomeChanges = true; }

  /// buffers for computations
  std::array<BufferTy, stripNbBuffers> _buffers;

private:
  volatile bool hasSomeChanges;

  /// Out strip brightness
  volatile uint8_t brightness;

  /// Store a reference to the brightness value from the last show() call
  volatile uint8_t brightnessAtShowTime;

  /// keep track of the show call count. Allowed to circle back to 0
  volatile uint8_t shownCount;

  /// used for temporal dithering
  std::array<uint32_t, useTemporalDithering ? LED_COUNT : 1> _colorErrors;

  /// store the display colors, with no brightness scaling
  std::array<uint32_t, LED_COUNT> _colors;
};

} // namespace component
} // namespace lampda

#endif

#endif
