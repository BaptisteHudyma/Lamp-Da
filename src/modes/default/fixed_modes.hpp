#ifndef FIXED_MODES_H
#define FIXED_MODES_H

#include "src/modes/include/colors/palettes.hpp"

namespace modes {

namespace fixed {

/// Single-color mode for indexable strips with palette ramp
struct FixedMode : public modes::FullMode {
  static void loop(auto& ctx) {
    uint32_t color = modes::colors::from_palette(
      ctx.get_active_custom_ramp(),
      ctx.state.palette);
    ctx.fill(color);
  }
};

//
// main lightning modes
//

/// Black-body fixed colors ramp mode
struct KelvinMode : public FixedMode {
  struct StateTy {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteBlackBodyColors;
  };

  // (enable the ramp to saturates, instead of wrapping ramp around)
  static void reset(auto& ctx) {
    ctx.set_custom_ramp_saturation(true);
  }
};

/// Rainbow fixed colors ramp mode
struct RainbowMode : public modes::FullMode {
  static void loop(auto& ctx) {
    const float index = ctx.get_active_custom_ramp();
    const float hue = (index / 256.f) * 360.f;
    uint32_t color = utils::hue_to_rgb_sinus(hue);

    ctx.fill(color);
  }
};

//
// optional "miscellaneous" group of other gradients
//

/// Party fixed colors ramp mode
struct PalettePartyMode : public FixedMode {
  struct StateTy {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePartyColors;
  };
};

/// Forest fixed colors ramp mode
struct PaletteForestMode : public FixedMode {
  struct StateTy {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteForestColors;
  };
};

/// Ocean fixed colors ramp mode
struct PaletteOceanMode : public FixedMode {
  struct StateTy {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteOceanColors;
  };
};

} // modes::fixed

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
