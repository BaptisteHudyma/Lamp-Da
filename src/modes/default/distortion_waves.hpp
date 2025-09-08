#ifndef DISTORTION_WAVE_MODE_H
#define DISTORTION_WAVE_MODE_H

/// @file distortion_wave.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/gamma.hpp"

/// Basic "default" modes included with the hardware
namespace modes::default_modes {

/// Emulate color circle waves propagating

// Distortion waves - ldirko
// https://editor.soulmatelights.com/gallery/1089-distorsion-waves
// adapted for WLED by @blazoncek
struct DistortionWaveMode : public BasicMode
{
  struct StateTy
  {
    uint16_t speed;
    uint16_t scale;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.speed = 128;
    ctx.state.scale = 128;
  }

  static void loop(auto& ctx)
  {
    const uint8_t _speed = ctx.state.speed / 32;
    const uint8_t _scale = ctx.state.scale / 32;

    const uint8_t w = 2;

    const uint16_t a = ctx.lamp.tick / 2;
    const uint16_t a2 = a / 2;
    const uint16_t a3 = a / 3;

    const uint16_t cx = beatsin8(10 - _speed, 0, ctx.lamp.maxWidth - 1) * _scale;
    const uint16_t cy = beatsin8(12 - _speed, 0, ctx.lamp.maxHeight - 1) * _scale;
    const uint16_t cx1 = beatsin8(13 - _speed, 0, ctx.lamp.maxWidth - 1) * _scale;
    const uint16_t cy1 = beatsin8(15 - _speed, 0, ctx.lamp.maxHeight - 1) * _scale;
    const uint16_t cx2 = beatsin8(17 - _speed, 0, ctx.lamp.maxWidth - 1) * _scale;
    const uint16_t cy2 = beatsin8(14 - _speed, 0, ctx.lamp.maxHeight - 1) * _scale;

    uint16_t xoffs = 0;
    for (int x = 0; x < ctx.lamp.maxWidth; x++)
    {
      xoffs += _scale;
      uint16_t yoffs = 0;

      for (int y = 0; y < ctx.lamp.maxHeight; y++)
      {
        yoffs += _scale;

        const uint8_t rdistort = cos8((cos8(((x << 3) + a) & 255) + cos8(((y << 3) - a2) & 255) + a3) & 255) >> 1;
        const uint8_t gdistort = cos8((cos8(((x << 3) - a2) & 255) + cos8(((y << 3) + a3) & 255) + a + 32) & 255) >> 1;
        const uint8_t bdistort = cos8((cos8(((x << 3) + a3) & 255) + cos8(((y << 3) - a) & 255) + a2 + 64) & 255) >> 1;

        const uint8_t red = rdistort + w * (a - (((xoffs - cx) * (xoffs - cx) + (yoffs - cy) * (yoffs - cy)) >> 7));
        const uint8_t green =
                gdistort + w * (a2 - (((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1)) >> 7));
        const uint8_t blue =
                bdistort + w * (a3 - (((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2)) >> 7));

        ctx.lamp.setPixelColorXY(x,
                                 y,
                                 colors::fromRGB(
                                         //
                                         colors::gamma8(cos8(red)),
                                         //
                                         colors::gamma8(cos8(green)),
                                         //
                                         colors::gamma8(cos8(blue))
                                         //
                                         ));
      }
    }
  }
};

} // namespace modes::default_modes

#endif
