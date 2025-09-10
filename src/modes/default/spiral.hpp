#ifndef SPIRAL_MODE_H
#define SPIRAL_MODE_H

/// @file spiral.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/palettes.hpp"

/// Basic "default" modes included with the hardware
namespace modes::default_modes {

// By: Stepko
// https://editor.soulmatelights.com/gallery/884-drift
// Modified by: Andrew Tuline
struct SpiralMode : public BasicMode
{
  struct StateTy
  {
    uint8_t fade;
    uint8_t intensity;
    uint8_t speed;
    palette_t palette;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.fade = 64;
    ctx.state.intensity = 64;
    ctx.state.speed = 250;
    ctx.state.palette = colors::PaletteRainbowColors;
  }

  static void loop(auto& ctx)
  {
    static constexpr uint16_t colsCenter = (ctx.lamp.maxWidth >> 1) + ctx.lamp.maxWidth % 2;
    static constexpr uint16_t rowsCenter = (ctx.lamp.maxHeight >> 1) + ctx.lamp.maxHeight % 2;
    static constexpr uint16_t maxDim = MAX(ctx.lamp.maxWidth, ctx.lamp.maxHeight) / 2;

    ctx.lamp.fadeToBlackBy(ctx.state.fade);
    unsigned long t = 4 * ctx.lamp.tick / (256 - ctx.state.speed);
    unsigned long t_20 = t / 20; // softhack007: pre-calculating this gives about 10% speedup
    for (float i = 1; i < maxDim; i += 0.25)
    {
      float angle = to_radians(t * (maxDim - i));
      uint16_t myX = colsCenter + (sin_t(angle) * i);
      uint16_t myY = rowsCenter + (cos_t(angle) * i);

      ctx.lamp.setPixelColorXY(myX, myY, colors::from_palette((uint8_t)((i * 20) + t_20), ctx.state.palette));
    }
    ctx.lamp.blur(ctx.state.intensity >> 3);
  }
};

} // namespace modes::default_modes

#endif
