#ifndef LEGACY_MODES_H
#define LEGACY_MODES_H

#include "src/system/charger/charger.h"
#include "src/system/utils/utils.h"
#include "src/system/colors/animations.h"
#include "src/system/colors/colors.h"
#include "src/system/colors/palettes.h"
#include "src/system/colors/wipes.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/IMU.h"
#include "src/system/physical/MicroPhone.h"

namespace modes::legacy {

/// Just a way to highlight which modes still uses src/system
using LegacyMode = FullMode;
using LegacySimpleMode = BasicMode;

//
// legacy calm modes
//

namespace calm {

/// Do a rainbow swirl!
struct RainbowSwirlMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& state = ctx.state;
    animations::fill(state.rainbowSwirl, ctx.strip);
    state.rainbowSwirl.update();
  }

  static void reset(auto& ctx) {
    ctx.state.rainbowSwirl.reset();
  }

  struct StateTy {
    GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);
  };
};

/// Fade slowly between PalettePartyColors
struct PartyFadeMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& state = ctx.state;
    state.isFinished = animations::fade_in(
      state.palettePartyColor, 100, state.isFinished, ctx.strip);

    if (state.isFinished) {
      state.palettePartyColor.update(++state.currentIndex);
    }
  }

  static void reset(auto& ctx) {
    auto& state = ctx.state;
    state.currentIndex = 0;
    state.palettePartyColor.reset();
  }

  struct StateTy {
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
template <typename T>
struct NoiseMode : public LegacyMode {
  static void reset(auto& ctx) {
    ctx.state.categoryChange = true;
  }

  struct StateTy {
    void random_noise(auto& strip, const palette_t& palette) {
      animations::random_noise(palette, strip, categoryChange, true, 3);
      categoryChange = false;
    }

    bool categoryChange = false;
  };
};

/// Noise from PaletteLavaColors
struct LavaNoiseMode : public NoiseMode<LavaNoiseMode> {
  static void loop(auto& ctx) {
    ctx.state.random_noise(ctx.strip, PaletteLavaColors);
  }
};

/// Noise from PaletteForestColors
struct ForestNoiseMode : public NoiseMode<ForestNoiseMode> {
  static void loop(auto& ctx) {
    ctx.state.random_noise(ctx.strip, PaletteForestColors);
  }
};

/// Noise from PaletteOceanColors
struct OceanNoiseMode : public NoiseMode<OceanNoiseMode> {
  static void loop(auto& ctx) {
    ctx.state.random_noise(ctx.strip, PaletteOceanColors);
  }
};

/// Polar lights
struct PolarMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& categoryChange = ctx.state.categoryChange;
    animations::mode_2DPolarLights(
      255, 128, PaletteAuroraColors, categoryChange, ctx.strip);
    categoryChange = false;
  }

  static void reset(auto& ctx) {
    ctx.state.categoryChange = true;
  }

  struct StateTy {
    bool categoryChange = false;
  };
};

/// Fireplace
struct FireMode : public LegacySimpleMode {
  static void loop(auto& strip) {
    animations::fire(60, 60, 255, PaletteHeatColors, strip);
  }
};

/// Rainbow sin waves
struct SineMode : public LegacySimpleMode {
  static void loop(auto& strip) {
    animations::mode_sinewave(128, 128, PaletteRainbowColors, strip);
  }
};

/// Bubbles
struct DriftMode : public LegacySimpleMode {
  static void loop(auto& strip) {
    animations::mode_2DDrift(64, 64, PaletteRainbowColors, strip);
  }
};

/// Distortion
struct DistMode : public LegacySimpleMode {
  static void loop(auto& strip) {
    animations::mode_2Ddistortionwaves(128, 128, strip);
  }
};

} // modes::legacy::calm

//
// legacy party modes
//

namespace party {

/// Wipe up and down complementary colors
struct ColorWipeMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& state = ctx.state;
    state.isFinished = state.switchMode ? (
        animations::color_wipe_up(state.complementaryColor, 500, state.isFinished, ctx.strip)
      ) : (
        animations::color_wipe_down(state.complementaryColor, 500, state.isFinished, ctx.strip)
      );

    if (state.isFinished) {
      state.switchMode = !state.switchMode;
      state.complementaryColor.update();
    }
  }

  static void reset(auto& ctx) {
    ctx.state.complementaryColor.reset();
  }

  struct StateTy {
    GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
    bool switchMode = false;
    bool isFinished = false;
  };
};

/// Animated random color fill from each side
struct RandomFillMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& state = ctx.state;
    state.isFinished = animations::double_side_fill(
      state.randomColor, 500, state.isFinished, ctx.strip);

    if (state.isFinished) {
      state.randomColor.update();
    }
  }

  static void reset(auto& ctx) {
    ctx.state.randomColor.reset();
  }

  struct StateTy {
    GenerateRandomColor randomColor = GenerateRandomColor();
    bool isFinished = false;
  };
};

/// Animated back-and-forth with random complementary color
struct PingPongMode : public LegacyMode {
  static void loop(auto& ctx) {
    auto& state = ctx.state;
    state.isFinished = animations::dot_ping_pong(
      state.complementaryPingPongColor, 1000, 128, state.isFinished, ctx.strip);

    if (state.isFinished) {
      state.complementaryPingPongColor.update();
    }
  }

  static void reset(auto& ctx) {
    ctx.state.complementaryPingPongColor.reset();
  }

  struct StateTy {
    GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.4);
    bool isFinished = false;
  };
};

} // modes::legacy::party

//
// Legacy modes groups
//

using CalmModes = modes::GroupFor<
  calm::RainbowSwirlMode,
  calm::PartyFadeMode,
  calm::LavaNoiseMode,
  calm::ForestNoiseMode,
  calm::OceanNoiseMode,
  calm::PolarMode,
  calm::FireMode,
  calm::SineMode,
  calm::DriftMode,
  calm::DistMode
>;

using PartyModes = modes::GroupFor<
  party::ColorWipeMode,
  party::RandomFillMode,
  party::PingPongMode
>;

} // modes::legacy

#endif
