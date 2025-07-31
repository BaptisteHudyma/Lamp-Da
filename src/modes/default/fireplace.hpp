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
struct FireMode : public BasicMode
{
  /// Fire custom ramp sets how sensitive it is to ambiant sound
  static constexpr bool hasCustomRamp = true;

  static constexpr uint8_t speed = 255;  ///< (1-255) How slow is your fire?
  static constexpr uint16_t xScale = 60; ///< Noise scaling (X direction)
  static constexpr uint16_t yScale = 60; ///< Noise scaling (Y direction)

  /// Palette used for fire colors
  static constexpr auto palette = colors::PaletteHeatColors;

  struct StateTy
  {
    uint32_t step;
    audio::SoundEventTy<> soundEvent;
  };

  static void reset(auto& ctx)
  {
    ctx.state.step = 0;
    ctx.state.soundEvent.reset(ctx);

    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    // tick forward
    ctx.state.step += 1;
    const int16_t zSpeed = ctx.state.step / (256 - speed);
    const int16_t ySpeed = time_ms() / (256 - speed);

    // measure custom ramp for fire sound sensitivity
    const float index = ctx.get_active_custom_ramp();
    if (index > 16)
    {
      ctx.state.soundEvent.update(ctx);
    }

    // measure sound level for sound-sensitive fire
    float shakeness = 1.0 + (index * ctx.state.soundEvent.avgDelta) / 255.0;
    float intensity = 128 + 256 / (1 + shakeness) - 1;

    // precompute "fire intensity" line per line
    intensity *= ctx.lamp.maxHeight;
    uint8_t decay[ctx.lamp.maxHeight] = {};

    for (int16_t j = 0; j < ctx.lamp.maxHeight; ++j)
    {
      const float here = MAX(intensity - j * 255.0, 0.0);
      decay[j] = MIN(here / ctx.lamp.maxHeight, 255.0);
    }

    // for each line, generate noise & set pixels
    for (uint16_t j = 0; j < ctx.lamp.maxHeight; ++j)
    {
      for (uint16_t i = 0; i < ctx.lamp.maxWidth; ++i)
      {
        const auto flame = noise8::inoise(i * xScale, j * yScale + ySpeed, zSpeed);
        const auto pixel = MIN(223, qsub8(flame, decay[j]));
        const auto color = modes::colors::from_palette<false, uint8_t>(pixel, palette);

        ctx.lamp.setPixelColorXY(i, j, color);
      }
    }
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    anims::rampColorRing(ctx, rampValue, colors::PaletteBlackBodyColors);
  }
};

} // namespace modes::default_modes

#endif
