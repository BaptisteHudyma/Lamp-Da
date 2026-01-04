#ifndef PALETTE_FADE_HPP
#define PALETTE_FADE_HPP

#include <cmath>
namespace modes::default_modes {

#include <cstdint>
#include "src/modes/include/colors/palettes.hpp"

struct PaletteFadeMode : public modes::BasicMode
{
  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  static void on_enter_mode(auto& ctx)
  {
    //
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);

    ctx.state.paletteIndex = 0;
  }

  static void loop(auto& ctx)
  {
    // ramp controls the speed
    static constexpr uint32_t lowestTiming = 20000;
    static constexpr uint32_t highestTiming = 3000;
    // map ramp to period
    const float wholePaletteLoopTiming =
            lmpd_map<float>(ctx.get_active_custom_ramp(), 0, 255, lowestTiming, highestTiming);

    // 1/80
    const float adding = wholePaletteLoopTiming / static_cast<float>(ctx.lamp.frameDurationMs);
    ctx.state.paletteIndex += 255.0 / adding;
    ctx.state.paletteIndex = std::fmod(ctx.state.paletteIndex, 255.0);

    ctx.lamp.fill(modes::colors::from_palette(static_cast<uint8_t>(ctx.state.paletteIndex), ctx.state.palette));
  }
};

/// Party fixed colors ramp mode
struct RainbowFadePaletteMode : public PaletteFadeMode
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteRainbowColors;
    float paletteIndex;
  };
};

} // namespace modes::default_modes

#endif
