#ifndef RAINBOW_SWIRL_MODE_HPP
#define RAINBOW_SWIRL_MODE_HPP

/// @file rainbow_swirl_mode.hpp

namespace modes::default_modes {

#include <cstdint>
#include "src/modes/include/colors/palettes.hpp"

/**
 * \brief Display a rainbow that moves
 */
struct RainbowSwirlMode : public BasicMode
{
  struct StateTy
  {
    /// per loop increment
    uint32_t increment;
    /// hue of the first pixel of the strip
    uint16_t firstPixelHue;
  };

  static void on_enter_mode(auto& ctx)
  {
    //
    static constexpr uint32_t animationPeriod_ms = 5000;
    ctx.state.increment = (UINT16_MAX / (animationPeriod_ms / ctx.lamp.frameDurationMs));
    ctx.state.firstPixelHue = 0;
  }

  static void loop(auto& ctx)
  {
    const uint16_t firstPixelColor = ctx.state.firstPixelHue;
    const float multiplier = UINT16_MAX / static_cast<float>(ctx.lamp.ledCount);
    for (size_t i = 0; i < ctx.lamp.ledCount; i++)
    {
      const uint16_t pixelHue = firstPixelColor + i * multiplier;
      uint32_t color = colors::fromAngleHue(lmpd_map<uint16_t>(pixelHue, 0, UINT16_MAX, 0, 360));
      ctx.lamp.setPixelColor(i, color);
    }

    // update first led color
    ctx.state.firstPixelHue += ctx.state.increment;
  }
};

} // namespace modes::default_modes

#endif
