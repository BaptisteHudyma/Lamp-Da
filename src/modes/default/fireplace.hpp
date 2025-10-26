#ifndef FIREPLACE_MODE_H
#define FIREPLACE_MODE_H

/// @file fireplace.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/anims/ramp_update.hpp"
#include "src/modes/include/audio/utils.hpp"

/// Basic "default" modes included with the hardware
namespace modes::default_modes {

/// Emulate a fireplace, optionally sound-sensitive

// Based on "Perlin noise fire procedure" - ldirko
// https://editor.soulmatelights.com/gallery/234-fire
struct FireMode : public BasicMode
{
  /// Fire custom ramp sets how sensitive it is to ambiant sound
  static constexpr bool hasCustomRamp = true;

  static constexpr uint16_t xScale = 60; ///< Noise scaling (X direction)
  static constexpr uint16_t yScale = 60; ///< Noise scaling (Y direction)

  /// Palette used for fire colors
  static constexpr auto palette = colors::PaletteHeatColors;

  struct StateTy
  {
    audio::SoundEventTy<> soundEvent;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.soundEvent.reset(ctx);

    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    // tick forward
    const int16_t zSpeed = ctx.lamp.tick;
    const int16_t ySpeed = zSpeed * ctx.lamp.frameDurationMs;

    // measure custom ramp for fire sound sensitivity
    const uint8_t index = ctx.get_active_custom_ramp();
    if (index > 16)
    {
      ctx.state.soundEvent.update(ctx);
    }

    // measure sound level for sound-sensitive fire
    float shakeness = 1.0 + (index * ctx.state.soundEvent.avgDelta) / 255.0;
    float intensity = 128 + 256 / (1 + shakeness) - 1;

    // precompute "fire intensity" line per line
    intensity *= ctx.lamp.maxHeight;

    // for each line, generate noise & set pixels
    for (uint16_t j = 0; j <= ctx.lamp.maxHeight; ++j)
    {
      const float here = MAX(intensity - j * 255.0, 0.0);
      const uint8_t decay = MIN(here / ctx.lamp.maxHeight, 255.0);

      for (uint16_t i = 0; i <= ctx.lamp.maxWidth; ++i)
      {
        const auto flame = noise8::inoise(i * xScale, j * yScale + ySpeed, zSpeed);
        const auto pixel = MIN(223, qsub8(flame, decay));
        const auto color = modes::colors::from_palette<false, uint8_t>(pixel, palette);

        ctx.lamp.setPixelColorXY(i, j, color);
      }
    }
  }
};

} // namespace modes::default_modes

#endif
