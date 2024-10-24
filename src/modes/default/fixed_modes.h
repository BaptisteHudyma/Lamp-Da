#ifndef FIXED_MODES_H
#define FIXED_MODES_H

struct KelvinMode : public modes::FullMode {

  static void loop(auto& ctx) {
    auto& state = ctx.state;
    auto& palette = state.palette;

    palette.update(state.color);
    state.color += 1;
    animations::fill(palette, ctx.strip);
  }

  static void reset(auto& ctx) {
    ctx.state.palette.reset();
    ctx.state.color = 0;
  }

  struct StateTy {
    GeneratePaletteIndexed palette = GeneratePaletteIndexed(PaletteBlackBodyColors);
    int color = 0;
  };

};

struct RainbowMode : public modes::FullMode {
  static void loop(auto& ctx) {
    static auto rainbowIndex = GenerateRainbowIndex(UINT8_MAX);
    rainbowIndex.reset();
    rainbowIndex.update(0);
    animations::fill(rainbowIndex, ctx.strip);
  }
};

using FixedModes = modes::GroupFor<KelvinMode, RainbowMode>;

#endif
