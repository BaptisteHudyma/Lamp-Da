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

#include "src/modes/default/aurora.hpp"
#include "src/modes/default/automaton.hpp"
#include "src/modes/default/distortion_waves.hpp"
#include "src/modes/default/fireplace.hpp"
#include "src/modes/default/perlin_noise.hpp"
#include "src/modes/default/sine_mode.hpp"
#include "src/modes/default/spiral.hpp"
#include "src/modes/default/vu_meter.hpp"

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

  static void on_enter_mode(auto& ctx) { ctx.state.rainbowSwirl.reset(); }

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

  static void on_enter_mode(auto& ctx)
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

  static void on_enter_mode(auto& ctx) { ctx.state.complementaryColor.reset(); }

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

  static void on_enter_mode(auto& ctx) { ctx.state.randomColor.reset(); }

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

  static void on_enter_mode(auto& ctx) { ctx.state.complementaryPingPongColor.reset(); }

  struct StateTy
  {
    GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.4);
    bool isFinished = false;
  };
};

} // namespace party

namespace sound {

struct FftMode : public LegacyMode
{
  static void loop(auto& ctx) { animations::fft_display(64, 64, PalettePartyColors, ctx.lamp.getLegacyStrip()); }

  static void on_enter_mode(auto& ctx) {}

  struct StateTy
  {
  };
};

} // namespace sound

namespace imu {

struct LiquideMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;

    if (state._color)
    {
      animations::liquid(state.persistance, *state._color, ctx.lamp.getLegacyStrip(), state.categoryChange);
      state._color->update();
      state.categoryChange = false;
    }
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    auto& state = ctx.state;

    // get color from ramp
    const uint8_t rampIndex =
            min<uint8_t>(floorf(rampValue / 255.0f * state.maxPalettesCount), state.maxPalettesCount - 1);
    state._color = state._colors[rampIndex];
  }

  static void on_enter_mode(auto& ctx)
  {
    auto& state = ctx.state;
    for (DynamicColor* color: state._colors)
    {
      color->reset();
    }
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(false);
    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
    state.categoryChange = true;
  }

  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    // store references to palettes
    DynamicColor* _colors[4] = {new GeneratePalette(2, PaletteOceanColors),
                                new GenerateRainbowSwirl(5000),
                                new GeneratePalette(2, PaletteAuroraColors),
                                new GeneratePalette(2, PaletteForestColors)};
    const uint8_t maxPalettesCount = 4;

    DynamicColor* _color;

    uint8_t persistance = 210;
    bool categoryChange = false;
  };
};

struct LiquideRainMode : public LegacyMode
{
  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    // rain is density mapped to the button ramp
    animations::rain(state.density, state.persistance, state.color, ctx.lamp.getLegacyStrip(), state.categoryChange);
    // using the update will change de color of the drops at each iteration
    state.color.update();
    state.categoryChange = false;
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue) { ctx.state.density = rampValue; }

  static void on_enter_mode(auto& ctx)
  {
    auto& state = ctx.state;
    state.color.reset();
    state.categoryChange = true;
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);

    // set default value
    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    GeneratePalette color = GeneratePalette(2, PaletteWaterColors);
    uint8_t persistance = 100;
    uint8_t density;
    bool categoryChange = false;
  };
};

} // namespace imu

//
// Legacy modes groups
//

using CalmModes = modes::GroupFor<calm::RainbowSwirlMode,
                                  calm::PartyFadeMode,
                                  default_modes::PerlinNoiseMode,
                                  default_modes::AuroraMode,
                                  default_modes::FireMode,
                                  default_modes::SineMode,
                                  default_modes::SpiralMode,
                                  default_modes::DistortionWaveMode,
                                  imu::LiquideMode,
                                  imu::LiquideRainMode,
                                  automaton::BubbleMode,
                                  automaton::SierpinskiMode>;

using PartyModes = modes::GroupFor<party::ColorWipeMode, party::RandomFillMode, party::PingPongMode>;

using SoundModes = modes::GroupFor<default_modes::VuMeterMode, sound::FftMode>;

} // namespace modes::legacy

#endif
