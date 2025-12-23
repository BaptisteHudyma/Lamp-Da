#ifndef LEGACY_MODES_H
#define LEGACY_MODES_H

#include "src/system/power/charger.h"

#include "src/system/utils/utils.h"

#include "src/system/colors/animations.h"
#include "src/system/colors/colors.h"
#include "src/system/colors/palettes.h"
#include "src/system/colors/wipes.h"

#include "src/system/physical/fileSystem.h"

#include "src/modes/default/aurora.hpp"
#include "src/modes/default/automaton.hpp"
#include "src/modes/default/distortion_waves.hpp"
#include "src/modes/default/fastFourrierTransform.hpp"
#include "src/modes/default/fireplace.hpp"
#include "src/modes/default/gravity_mode.hpp"
#include "src/modes/default/perlin_noise.hpp"
#include "src/modes/default/rain_mode.hpp"
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
  static constexpr bool hasCustomRamp = true;

  static void loop(auto& ctx)
  {
    const uint32_t rampIndex = lmpd_map<float>(ctx.get_active_custom_ramp(), 0, 255, 30.0, 500.0);

    auto& state = ctx.state;
    state.isFinished =
            animations::fade_in(state.palettePartyColor, rampIndex, state.isFinished, ctx.lamp.getLegacyStrip());

    if (state.isFinished)
    {
      state.palettePartyColor.update(++state.currentIndex);
    }
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);

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
                                  default_modes::GravityMode,
                                  default_modes::RainMode,
                                  automaton::BubbleMode,
                                  automaton::SierpinskiMode>;

using PartyModes = modes::GroupFor<party::ColorWipeMode, party::RandomFillMode, party::PingPongMode>;

using SoundModes = modes::GroupFor<default_modes::VuMeterMode, default_modes::FastFourrierTransformMode>;

} // namespace modes::legacy

#endif
