#ifndef SINE_MODE_H
#define SINE_MODE_H

/// @file sine_mode.hpp
/// Based on the Adjustable sinewave. By Andrew Tuline

#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/colors/palettes.hpp"

namespace lampda::modes::default_modes {

/**
 * \brief Barber shop sign looking animation, with nice swirl effect.
 */
struct SineMode : public modes::BasicMode
{
  /// speed of the animation
  static constexpr uint8_t speed = 128;

  /// User ramp change the frequency of the mode
  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    /// step of the animation
    uint16_t step;
    /// color palette to use
    colors::PaletteTy palette;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.step = 0;
    ctx.state.palette = colors::PaletteRainbowColors;

    // the ramp will not return to 0 after reach 255 (and the oposite)
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    const uint16_t colorIndex = ctx.lamp.tick / 4; // Amount of colour change.
    const float rampIndex = ctx.get_active_custom_ramp() / 255.0;
    const float freq = 128 / 4.0 + rampIndex * 16 / 4.0; // Frequency of the signal.

    ctx.state.step += speed / 16; // Speed of animation.
    const float colorIndexNormalised = colorIndex / 255.0;

    for (size_t i = 0; i < ctx.lamp.ledCount; i++)
    {
      // For each of the LED's in the strip, set a brightness based on a wave as follows:
      // cubicwave8 is 8 bits, so value will be truncated
      const uint8_t pixBri = cubicwave8((i * freq) + ctx.state.step);

      // get the pixel color from palette
      const auto pixColor = colors::from_palette((uint8_t)(i * colorIndexNormalised), ctx.state.palette);
      // blend the pixel color with the black color
      ctx.lamp.setPixelColor(i, modes::colors::fade<false>(pixColor, pixBri));
    }
  }
};

} // namespace lampda::modes::default_modes
#endif
