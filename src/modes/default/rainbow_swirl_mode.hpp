#ifndef RAINBOW_SWIRL_MODE_HPP
#define RAINBOW_SWIRL_MODE_HPP

namespace modes::default_modes {

#include <cstdint>
#include "src/modes/include/colors/palettes.hpp"

struct RainbowSwirlMode : public BasicMode
{
  struct StateTy
  {
    uint32_t increment;
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
