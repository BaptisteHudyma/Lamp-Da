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
#include "src/modes/default/color_wipe_mode.hpp"
#include "src/modes/default/distortion_waves.hpp"
#include "src/modes/default/double_side_fill_mode.hpp"
#include "src/modes/default/fastFourrierTransform.hpp"
#include "src/modes/default/fireplace.hpp"
#include "src/modes/default/gravity_mode.hpp"
#include "src/modes/default/palette_fade_mode.hpp"
#include "src/modes/default/perlin_noise.hpp"
#include "src/modes/default/rain_mode.hpp"
#include "src/modes/default/rainbow_swirl_mode.hpp"
#include "src/modes/default/sine_mode.hpp"
#include "src/modes/default/spiral.hpp"
#include "src/modes/default/vu_meter.hpp"

namespace modes::legacy {

/// Just a way to highlight which modes still uses src/system
using LegacyMode = BasicMode;

//
// legacy party modes
//

namespace party {

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

using CalmModes = modes::GroupFor<default_modes::RainbowSwirlMode,
                                  default_modes::RainbowFadePaletteMode,
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

using PartyModes =
        modes::GroupFor<default_modes::ColorWipeMode, default_modes::DoubleSideFillMode, party::PingPongMode>;

using SoundModes = modes::GroupFor<default_modes::VuMeterMode, default_modes::FastFourrierTransformMode>;

} // namespace modes::legacy

#endif
