#ifndef FIXED_MODES_H
#define FIXED_MODES_H

/// @file fixed_modes.hpp

#include "src/modes/include/colors/palettes.hpp"

#include "src/modes/include/anims/fadeout.hpp"

namespace lampda {

namespace modes {

namespace fixed {

/**
 * \brief Black-body fixed colors ramp mode
 */
struct KelvinMode : public modes::BasicMode
{
  static constexpr uint8_t dissolveBufferId = 0;
  inline static anims::fadeout::GravityDissolve<dissolveBufferId> sunsetAnimation =
          anims::fadeout::GravityDissolve<dissolveBufferId>();

  static void loop(auto& ctx)
  {
    // clear previous animation
    ctx.lamp.clear();

    const uint32_t color = colors::fromTemp(ctx.get_active_custom_ramp());
    // update animation
    sunsetAnimation.loop(ctx, color);

    ctx.lamp.fill(color, ctx.lamp.template getTempBuffer<dissolveBufferId>());
  }

  // configure custom ramp
  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);

    sunsetAnimation.reset(ctx);
  }

  /// Sunset timer will drop pixels downward, and never display them again
  static void sunset_update(auto& ctx, float progress) { sunsetAnimation.update_depop_rate(ctx, progress); }

  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
  /// sunset animation on the fixed modes
  static constexpr bool hasSunsetAnimation = true;
};

/**
 * \brief Rainbow fixed colors ramp mode
 */
struct RainbowMode : public modes::BasicMode
{
  static constexpr uint8_t dissolveBufferId = 0;
  inline static anims::fadeout::GravityDissolve<dissolveBufferId> sunsetAnimation =
          anims::fadeout::GravityDissolve<dissolveBufferId>();

  // configure custom ramp
  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);
    // this ramps should be slow
    ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(50);

    sunsetAnimation.reset(ctx);
  }

  static void loop(auto& ctx)
  {
    // clear previous animation
    ctx.lamp.clear();

    const uint8_t index = ctx.get_active_custom_ramp();
    const float hue = (index / 256.f) * 360.f;
    uint32_t color = colors::fromAngleHue(hue);

    // update animation
    sunsetAnimation.loop(ctx, color);
    // fill, with mask
    ctx.lamp.fill(color, ctx.lamp.template getTempBuffer<dissolveBufferId>());
  }

  /// Sunset timer will drop pixels downward, and never display them again
  static void sunset_update(auto& ctx, float progress) { sunsetAnimation.update_depop_rate(ctx, progress); }

  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
  /// sunset animation on the fixed modes
  static constexpr bool hasSunsetAnimation = true;
};

/**
 * \brief Single-color mode for indexable strips with palette ramp
 * \param[in] isStep Set to false, the palette will be interpolated. Set to true, the colors will step through the
 * palette.
 */
template<bool isStep = false> struct PaletteMode : public modes::BasicMode
{
  static constexpr uint8_t dissolveBufferId = 0;
  inline static anims::fadeout::GravityDissolve<dissolveBufferId> sunsetAnimation =
          anims::fadeout::GravityDissolve<dissolveBufferId>();

  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);
    // this ramps should be slow
    ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(16 / 255.0f * 850);

    sunsetAnimation.reset(ctx);
  }

  static void loop(auto& ctx)
  {
    // clear previous animation
    ctx.lamp.clear();

    const uint8_t customRamp = ctx.get_active_custom_ramp();
    uint32_t color = modes::colors::from_palette(
            // if palette is stepped, remove all intermediate colors
            static_cast<uint8_t>(isStep ? (customRamp >> 4) << 4 : customRamp),
            // not stepped, allow interpolation
            ctx.state.palette);

    // update animation
    sunsetAnimation.loop(ctx, color);
    // fill, with mask
    ctx.lamp.fill(color, ctx.lamp.template getTempBuffer<dissolveBufferId>());
  }

  /// Sunset timer will drop pixels downward, and never display them again
  static void sunset_update(auto& ctx, float progress) { sunsetAnimation.update_depop_rate(ctx, progress); }

  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
  /// sunset animation on the fixed modes
  static constexpr bool hasSunsetAnimation = true;
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
} // namespace lampda

#endif
