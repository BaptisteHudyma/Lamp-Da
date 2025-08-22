#ifndef SINE_MODE_H
#define SINE_MODE_H

// Base on the Adjustable sinewave. By Andrew Tuline

#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/colors/palettes.hpp"
namespace modes::default_modes {

struct SineMode : public modes::BasicMode
{
  static constexpr uint8_t speed = 128;

  // change the frequency of the mode
  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    uint16_t step;
  };

  static void reset(auto& ctx)
  {
    ctx.state.step = 0;

    // the ramp will not return to 0 after reach 255 (and the oposite)
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    uint8_t intensity = 128;
    uint16_t colorIndex = ctx.lamp.tick / 4; // Amount of colour change.
    const uint8_t index = ctx.get_active_custom_ramp() / 4;

    ctx.state.step += speed / 16;            // Speed of animation.
    uint16_t freq = (intensity + index) / 4; // Frequency of the signal.

    for (int i = 0; i < LED_COUNT; i++)
    { // For each of the LED's in the strand, set a brightness based on
      // a wave as follows:
      uint8_t pixBri = cubicwave8((i * freq) + ctx.state.step); // qsuba(cubicwave8((i*freq)+SEGENV.step),
                                                                // (255-SEGMENT.intensity)); // qsub sets a minimum
                                                                // value called thiscutoff. If < thiscutoff, then bright
                                                                // = 0. Otherwise, bright = 128 (as defined in qsub)..
      // get the pixel color from PaletteRainbowColors
      const auto pixColor = modes::colors::from_palette((uint8_t)(i * colorIndex / 255), PaletteRainbowColors);
      // blend the pixel color with the black color
      ctx.lamp.setPixelColor(i, modes::colors::fade<false>(pixColor, pixBri));
    }
  }
};

} // namespace modes::default_modes
#endif
