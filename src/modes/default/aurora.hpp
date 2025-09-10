#ifndef AURORA_MODE_H
#define AURORA_MODE_H

/// @file aurora.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/palettes.hpp"

/// Basic "default" modes included with the hardware
namespace modes::default_modes {

/// Aurora borealis effect

// By: Kostyantyn Matviyevskyy
// https://editor.soulmatelights.com/gallery/762-polar-lights
// , Modified by: Andrew Tuline
struct AuroraMode : public BasicMode
{
  struct StateTy
  {
    uint8_t scale;
    uint8_t speed;
    uint32_t step;
    palette_t palette;
  };

  static void on_enter_mode(auto& ctx)
  {
    static constexpr uint16_t adjScale = linear_scale<uint32_t, uint16_t>(ctx.lamp.maxWidth + 1, 8, 64, 310, 63);

    static constexpr uint8_t scale = 255;
    static constexpr uint8_t speed = 128;

    ctx.state.scale = linear_scale<uint8_t, uint16_t>(scale, 0, 255, 30, adjScale);
    ctx.state.speed = linear_scale<byte, byte>(speed, 0, 255, 128, 16);
    ctx.state.step = 0;
    ctx.state.palette = colors::PaletteAuroraColors;
  }

  static void loop(auto& ctx)
  {
    static constexpr float adjustHeight = linear_scale<uint32_t, float>(ctx.lamp.maxHeight, 8, 32, 28, 12);
    static constexpr float halfHeight = (float)ctx.lamp.maxHeight / 2.0f;

    const float _speedDivider = 1.0f / static_cast<float>(ctx.state.speed);
    const auto& palette = ctx.state.palette;

    uint32_t step = ctx.state.step;
    for (int x = 0; x < ctx.lamp.maxWidth + 1; x++)
    {
      const int scaledX = x * ctx.state.scale;
      for (int y = 0; y < ctx.lamp.maxHeight; y++, step++)
      {
        const auto& color = colors::from_palette(
                qsub8(noise8::inoise((step % 2) + scaledX, y * 16 + step % 16, step * _speedDivider),
                      fabsf(halfHeight - (float)y) * adjustHeight),
                palette);
        ctx.lamp.setPixelColorXY(x, y, color);
      }
    }

    // update steps
    ctx.state.step = step;
  }
};

} // namespace modes::default_modes

#endif
