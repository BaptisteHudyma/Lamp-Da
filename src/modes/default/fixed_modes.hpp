#ifndef FIXED_MODES_H
#define FIXED_MODES_H

#include "src/modes/include/colors/palettes.hpp"

namespace modes {

namespace fixed {

//
// main lightning modes
//

/// Black-body fixed colors ramp mode
struct KelvinMode : public modes::BasicMode
{
  static void loop(auto& ctx) { ctx.lamp.setLightTemp(ctx.get_active_custom_ramp()); }

  // (force ramp to saturates, instead of wrapping ramp around)
  static void reset(auto& ctx) { ctx.template set_config_bool<ConfigKeys::rampSaturates>(true); }
};

/// Rainbow fixed colors ramp mode
struct RainbowMode : public modes::BasicMode
{
  static void loop(auto& ctx)
  {
    const float index = ctx.get_active_custom_ramp();
    const float hue = (index / 256.f) * 360.f;
    uint32_t color = colors::fromAngleHue(hue);

    ctx.lamp.fill(color);
  }
};

//
// optional "miscellaneous" group of other gradients
//

/// Single-color mode for indexable strips with palette ramp
struct PaletteMode : public modes::BasicMode
{
  static void loop(auto& ctx)
  {
    uint32_t color = modes::colors::from_palette(ctx.get_active_custom_ramp(), ctx.state.palette);
    ctx.lamp.fill(color);
  }
};

/// Party fixed colors ramp mode
struct PalettePartyMode : public PaletteMode
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePartyColors;
  };
};

/// Forest fixed colors ramp mode
struct PaletteForestMode : public PaletteMode
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteForestColors;
  };
};

/// Ocean fixed colors ramp mode
struct PaletteOceanMode : public PaletteMode
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteOceanColors;
  };
};

} // namespace fixed

//
// Fixed modes groups
//

using FixedModes = modes::GroupFor<
  fixed::KelvinMode,
  fixed::RainbowMode
>;

using MiscFixedModes = modes::GroupFor<
  fixed::PalettePartyMode,
  fixed::PaletteForestMode,
  fixed::PaletteOceanMode
>;

} // modes

#endif
