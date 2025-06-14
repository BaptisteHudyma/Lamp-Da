#ifndef LEGACY_MODES_H
#define LEGACY_MODES_H

#include "src/system/power/charger.h"

#include "src/system/utils/utils.h"

#include "src/system/colors/animations.h"
#include "src/system/colors/colors.h"
#include "src/system/colors/palettes.h"
#include "src/system/colors/soundAnimations.h"
#include "src/system/colors/imuAnimations.h"
#include "src/system/colors/wipes.h"

#include "src/system/physical/fileSystem.h"

namespace modes::legacy {

/// Just a way to highlight which modes still uses src/system
using LegacyMode = BasicMode;

//
// legacy calm modes
//

namespace calm {

/// Do a rainbow swirl!
struct RainbowSwirlMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    animations::fill(state.rainbowSwirl, ctx.lamp.getLegacyStrip());
    state.rainbowSwirl.update();
  }

  static void reset(auto& ctx) { ctx.state.rainbowSwirl.reset(); }

  struct StateTy
  {
    GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);
  };
};

/// Fade slowly between PalettePartyColors
struct PartyFadeMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    state.isFinished = animations::fade_in(state.palettePartyColor, 100, state.isFinished, ctx.lamp.getLegacyStrip());

    if (state.isFinished)
    {
      state.palettePartyColor.update(++state.currentIndex);
    }
  }

  static void reset(auto& ctx)
  {
    auto& state = ctx.state;
    state.currentIndex = 0;
    state.palettePartyColor.reset();
  }

  struct StateTy
  {
    GeneratePaletteIndexed palettePartyColor = GeneratePaletteIndexed(PalettePartyColors);
    uint8_t currentIndex = 0;
    bool isFinished = false;
  };
};

/** \brief Parent class for legacy noise modes
 *
 * Inherit from `NoiseMode<YourMode>`
 *  - this enable all derived `StateTy` to be unique
 */
template<typename T> struct NoiseMode : public LegacyMode
{
  static void reset(auto& ctx) { ctx.state.categoryChange = true; }

  struct StateTy
  {
    void random_noise(auto& lamp, const palette_t& palette)
    {
      animations::random_noise(palette, lamp.getLegacyStrip(), categoryChange, true, 600);
      categoryChange = false;
    }

    bool categoryChange = false;
  };
};

/// Noise from PaletteLavaColors
struct LavaNoiseMode : public NoiseMode<LavaNoiseMode>
{
  static void loop(auto& ctx) { ctx.state.random_noise(ctx.lamp, PaletteLavaColors); }
};

/// Noise from PaletteForestColors
struct ForestNoiseMode : public NoiseMode<ForestNoiseMode>
{
  static void loop(auto& ctx) { ctx.state.random_noise(ctx.lamp, PaletteForestColors); }
};

/// Noise from PaletteOceanColors
struct OceanNoiseMode : public NoiseMode<OceanNoiseMode>
{
  static void loop(auto& ctx) { ctx.state.random_noise(ctx.lamp, PaletteOceanColors); }
};

/// Polar lights
struct PolarMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& categoryChange = ctx.state.categoryChange;
    animations::mode_2DPolarLights(255, 128, PaletteAuroraColors, categoryChange, ctx.lamp.getLegacyStrip());
    categoryChange = false;
  }

  static void reset(auto& ctx) { ctx.state.categoryChange = true; }

  struct StateTy
  {
    bool categoryChange = false;
  };
};

/// Fireplace
struct FireMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::fire(60, 60, 255, PaletteHeatColors, ctx.lamp.getLegacyStrip()); }
};

/// Rainbow sin waves
struct SineMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::mode_sinewave(128, 128, PaletteRainbowColors, ctx.lamp.getLegacyStrip()); }
};

/// Bubbles
struct DriftMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::mode_2DDrift(64, 64, PaletteRainbowColors, ctx.lamp.getLegacyStrip()); }
};

/// Distortion
struct DistMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::mode_2Ddistortionwaves(128, 128, ctx.lamp.getLegacyStrip()); }
};

} // namespace calm

//
// legacy party modes
//

namespace party {

/// Wipe up and down complementary colors
struct ColorWipeMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    auto& legacyStrip = ctx.lamp.getLegacyStrip();

    state.isFinished =
            state.switchMode ?
                    (animations::color_wipe_up(state.complementaryColor, 500, state.isFinished, legacyStrip)) :
                    (animations::color_wipe_down(state.complementaryColor, 500, state.isFinished, legacyStrip));

    if (state.isFinished)
    {
      state.switchMode = !state.switchMode;
      state.complementaryColor.update();
    }
  }

  static void reset(auto& ctx) { ctx.state.complementaryColor.reset(); }

  struct StateTy
  {
    GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
    bool switchMode = false;
    bool isFinished = false;
  };
};

/// Animated random color fill from each side
struct RandomFillMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    state.isFinished =
            animations::double_side_fill(state.randomColor, 500, state.isFinished, ctx.lamp.getLegacyStrip());

    if (state.isFinished)
    {
      state.randomColor.update();
    }
  }

  static void reset(auto& ctx) { ctx.state.randomColor.reset(); }

  struct StateTy
  {
    GenerateRandomColor randomColor = GenerateRandomColor();
    bool isFinished = false;
  };
};

/// Animated back-and-forth with random complementary color
struct PingPongMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    state.isFinished = animations::dot_ping_pong(
            state.complementaryPingPongColor, 1000, 128, state.isFinished, ctx.lamp.getLegacyStrip());

    if (state.isFinished)
    {
      state.complementaryPingPongColor.update();
    }
  }

  static void reset(auto& ctx) { ctx.state.complementaryPingPongColor.reset(); }

  struct StateTy
  {
    GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.4);
    bool isFinished = false;
  };
};

} // namespace party

namespace sound {

struct VuMeterMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    animations::vu_meter(state.redToGreenGradient, 128, ctx.lamp.getLegacyStrip());
  }

  static void reset(auto& ctx)
  {
    auto& state = ctx.state;
    state.redToGreenGradient.reset();
  }

  struct StateTy
  {
    GenerateGradientColor redToGreenGradient = GenerateGradientColor(
            LedStrip::Color(0, 255, 0), LedStrip::Color(255, 0, 0)); // gradient from red to green};
  };
};

struct FftMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::fft_display(64, 64, PalettePartyColors, ctx.lamp.getLegacyStrip()); }

  static void reset(auto& ctx) {}

  struct StateTy
  {
  };
};

} // namespace sound

namespace imu {

struct RainbowLiquideMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    animations::liquid(state.persistance, state.color, ctx.lamp.getLegacyStrip(), false);
    state.color.update();
  }

  static void reset(auto& ctx)
  {
    auto& state = ctx.state;
    state.color.reset();
    animations::liquid(state.persistance, state.color, ctx.lamp.getLegacyStrip(), true);
  }

  struct StateTy
  {
    GenerateRainbowSwirl color = GenerateRainbowSwirl(5000);
    uint8_t persistance = 210;
  };
};

struct PaletteLiquideMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    animations::liquid(state.persistance, state.color, ctx.lamp.getLegacyStrip(), false);
    state.color.update();
  }

  static void reset(auto& ctx)
  {
    auto& state = ctx.state;
    state.color.reset();
    animations::liquid(state.persistance, state.color, ctx.lamp.getLegacyStrip(), true);
  }

  struct StateTy
  {
    GeneratePalette color = GeneratePalette(2, PaletteForestColors);
    uint8_t persistance = 210;
  };
};

struct LiquideRainMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    // rain density mapped to the button ramp
    animations::rain(ctx.get_active_custom_ramp(), state.persistance, state.color, ctx.lamp.getLegacyStrip(), false);
    // using the update will change de color of the drops at each iteration
    state.color.update();
  }

  static void reset(auto& ctx)
  {
    auto& state = ctx.state;
    state.color.reset();
    animations::rain(1, state.persistance, state.color, ctx.lamp.getLegacyStrip(), true);
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    GeneratePalette color = GeneratePalette(2, PaletteCloudColors);
    uint8_t persistance = 100;
  };
};

} // namespace imu

//
// Legacy modes groups
//

using CalmModes = modes::GroupFor<calm::RainbowSwirlMode,
                                  calm::PartyFadeMode,
                                  calm::LavaNoiseMode,
                                  calm::ForestNoiseMode,
                                  calm::OceanNoiseMode,
                                  calm::PolarMode,
                                  calm::FireMode,
                                  calm::SineMode,
                                  calm::DriftMode,
                                  calm::DistMode>;

using PartyModes = modes::GroupFor<party::ColorWipeMode, party::RandomFillMode, party::PingPongMode>;

using SoundModes = modes::GroupFor<sound::VuMeterMode, sound::FftMode>;

using ImuModes = modes::GroupFor<imu::RainbowLiquideMode, imu::PaletteLiquideMode, imu::LiquideRainMode>;

} // namespace modes::legacy

#endif
