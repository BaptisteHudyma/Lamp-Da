#ifndef FIXED_MODES_H
#define FIXED_MODES_H

struct KelvinMode : public modes::FullMode {
  static void loop(auto& ctx) {
    static auto paletteHeatColor = GeneratePaletteIndexed(PaletteBlackBodyColors);
    paletteHeatColor.reset();
    paletteHeatColor.update(0);
    animations::fill(paletteHeatColor, ctx.strip);
  }
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
