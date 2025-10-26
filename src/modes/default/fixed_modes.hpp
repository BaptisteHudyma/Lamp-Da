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

  // configure custom ramp
  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);
  }

  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
};

/// Rainbow fixed colors ramp mode
struct RainbowMode : public modes::BasicMode
{
  // configure custom ramp
  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);
    // this ramps should be slow
    ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(50);
  }

  static void loop(auto& ctx)
  {
    const uint8_t index = ctx.get_active_custom_ramp();
    const float hue = (index / 256.f) * 360.f;
    uint32_t color = colors::fromAngleHue(hue);

    ctx.lamp.fill(color);
  }

  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
};

//
// optional "miscellaneous" group of other gradients
//

/// Single-color mode for indexable strips with palette ramp
template<bool isStep = false> struct PaletteMode : public modes::BasicMode
{
  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);
    // this ramps should be slow
    ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(16 / 255.0f * 850);
  }

  static void loop(auto& ctx)
  {
    const uint8_t customRamp = ctx.get_active_custom_ramp();
    uint32_t color = modes::colors::from_palette(
            // if palette is stepped, remove all intermediate colors
            static_cast<uint8_t>(isStep ? (customRamp >> 4) << 4 : customRamp),
            // not stepped, allow interpolation
            ctx.state.palette);
    ctx.lamp.fill(color);
  }

  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
};

/// Party fixed colors ramp mode
struct PalettePartyMode : public PaletteMode<false>
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePartyColors;
  };
};

/// Forest fixed colors ramp mode
struct PaletteForestMode : public PaletteMode<false>
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteForestColors;
  };
};

/// Ocean fixed colors ramp mode
struct PaletteOceanMode : public PaletteMode<false>
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteOceanColors;
  };
};

struct PalettePapiMode : public PaletteMode<true>
{
  struct StateTy
  {
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePapiColors;
  };
};

} // namespace fixed

//
// Fixed modes groups
//

using FixedModes = modes::GroupFor<fixed::KelvinMode, fixed::RainbowMode, fixed::PalettePapiMode>;

using MiscFixedModes = modes::GroupFor<fixed::PalettePartyMode, fixed::PaletteForestMode, fixed::PaletteOceanMode>;

} // namespace modes

#endif
