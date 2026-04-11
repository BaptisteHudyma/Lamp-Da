#ifndef FIXED_MODES_H
#define FIXED_MODES_H

#include "src/modes/include/colors/palettes.hpp"

namespace modes {

namespace fixed {

/**
 * \brief Black-body fixed colors ramp mode
 */
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

/**
 * \brief Rainbow fixed colors ramp mode
 */
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

/**
 * \brief Single-color mode for indexable strips with palette ramp
 * \param[in] isStep Set to false, the palette will be interpolated. Set to true, the colors will step through the
 * palette.
 */
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

  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
};

/**
 * \brief Party fixed colors ramp mode
 */
struct PalettePartyMode : public PaletteMode<false>
{
  struct StateTy
  {
    /// Color palette to use
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePartyColors;
  };
};

/**
 * \brief Forest fixed colors ramp mode
 */
struct PaletteForestMode : public PaletteMode<false>
{
  struct StateTy
  {
    /// Color palette to use
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteForestColors;
  };
};

/**
 * \brief Ocean fixed colors ramp mode
 */
struct PaletteOceanMode : public PaletteMode<false>
{
  struct StateTy
  {
    /// Color palette to use
    static constexpr modes::colors::PaletteTy palette = modes::colors::PaletteOceanColors;
  };
};

/**
 * \brief Special colored palette step mode. Ideal to set a target prefered color
 */
struct PalettePapiMode : public PaletteMode<true>
{
  struct StateTy
  {
    /// Color palette to use
    static constexpr modes::colors::PaletteTy palette = modes::colors::PalettePapiColors;
  };
};

} // namespace fixed

/// Fixed modes groups
using FixedModes = modes::GroupFor<fixed::KelvinMode, fixed::RainbowMode, fixed::PalettePapiMode>;
/// Fixed palette colors modes
using MiscFixedModes = modes::GroupFor<fixed::PalettePartyMode, fixed::PaletteForestMode, fixed::PaletteOceanMode>;

} // namespace modes

#endif
