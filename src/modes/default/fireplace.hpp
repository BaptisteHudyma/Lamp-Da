#ifndef FIREPLACE_MODE_H
#define FIREPLACE_MODE_H

/// @file fireplace.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/audio/utils.hpp"

#include "src/modes/include/colors/palettes.hpp"

#include <cstdint>

/// Basic "default" modes included with the hardware
namespace lampda::modes::default_modes {

/**
 * \brief Emulate a fireplace, optionally sound-sensitive.
 * Based on "Perlin noise fire procedure" - ldirko.
 * https://editor.soulmatelights.com/gallery/234-fire
 */
struct FireMode : public BasicMode
{
  /// Fire custom ramp sets how sensitive it is to ambiant sound
  static constexpr bool hasCustomRamp = true;

  static constexpr uint16_t xScale = 60; ///< Noise scaling (X direction)
  static constexpr uint16_t yScale = 60; ///< Noise scaling (Y direction)

  /// Palette used for fire colors
  static constexpr auto palette = colors::PaletteHeatColors;

  // this is too heavy to run at full, speed, display every other pixels instead of refreshing amm
  static constexpr uint32_t everyNIndex = 2;

  struct StateTy
  {
    /// handle sound event
    audio::SoundEventTy<> soundEvent;

    // flag that we have just been reseted
    bool isResetted = false;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.isResetted = true;
    ctx.state.soundEvent.reset(ctx);

    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    if (ctx.state.isResetted)
    {
      ctx.state.isResetted = false;

      // load animation
      for (uint32_t i = 0; i <= everyNIndex; ++i)
      {
        fire_display(ctx, i);
      }
      return;
    }

    fire_display(ctx, ctx.lamp.tick);
  }

  static void fire_display(auto& ctx, const uint32_t tick)
  {
    // tick forward
    const int16_t zSpeed = tick % INT16_MAX;
    const int16_t ySpeed = zSpeed * ctx.lamp.frameDurationMs;

    // measure custom ramp for fire sound sensitivity
    float soundAverageDelta = 0.0;
    const uint8_t index = ctx.get_active_custom_ramp();
    if (index > 16)
    {
      ctx.state.soundEvent.update(ctx);
      soundAverageDelta = ctx.state.soundEvent.avgDelta * 100.0;
    }

    // measure sound level for sound-sensitive fire
    float shakeness = 1.0 + (index * soundAverageDelta) / 255.0;
    float intensity = 128 + 256 / (1 + shakeness) - 1;

    // precompute "fire intensity" line per line
    intensity *= ctx.lamp.maxHeight;

    const size_t firstIndex = tick % everyNIndex;

    // for each line, generate noise & set pixels
    for (uint16_t j = firstIndex; j <= ctx.lamp.maxHeight; j += everyNIndex)
    {
      const float here = std::max<float>(intensity - j * 255.0, 0.0);
      const uint8_t decay = std::min<uint8_t>(here / ctx.lamp.maxHeight, 255.0);

      for (uint16_t i = 0; i <= ctx.lamp.maxWidth; ++i)
      {
        const auto flame = noise8::inoise(i * xScale, j * yScale + ySpeed, zSpeed);
        const auto pixel = std::min<uint8_t>(223, qsub8(flame, decay));
        const auto color = modes::colors::from_palette<false, uint8_t>(pixel, palette);

        ctx.lamp.setPixelColorXY(i, j, color);
      }
    }
  }
};

} // namespace lampda::modes::default_modes

#endif
