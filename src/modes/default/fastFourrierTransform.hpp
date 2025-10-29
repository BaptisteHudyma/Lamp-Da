#ifndef FFT_MODE_H
#define FFT_MODE_H

#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/audio/utils.hpp"

namespace modes::default_modes {

struct FastFourrierTransformMode : public BasicMode
{
  static constexpr bool hasCustomRamp = true;
  static constexpr auto palette = colors::PalettePartyColors;

  static void loop(auto& ctx)
  {
    const uint16_t cols = ctx.lamp.maxWidth - 1;
    const uint16_t rows = ctx.lamp.maxHeight;
    auto& state = ctx.state;
    state.soundEvent.update(ctx);

    const auto& fft = state.soundEvent.fft;
    float maxFftVal = 0.0;
    for (const float val: fft)
      maxFftVal = max(maxFftVal, val);

    // adjust for volume
    const float maxLevel = lmpd_map<float, float>(
            ctx.state.soundEvent.level, microphone::silenceLevelDb, microphone::highLevelDb, 0.0, 1.0);
    const float maxDisplaylevel = lmpd_constrain(maxLevel * rows, 2, rows);

    for (uint8_t x = 0; x < cols; ++x)
    {
      const uint8_t mappedX =
              lmpd_map<uint8_t, uint8_t>(x, 0, cols, 0, microphone::SoundStruct::numberOfFFtChanels - 1);
      const uint8_t mappedY = lmpd_constrain(
              lmpd_map<float, uint8_t>(fft[mappedX], 0, maxFftVal, 2, maxDisplaylevel), 2, maxDisplaylevel);
      for (uint8_t y = 0; y <= mappedY; y++)
      {
        uint8_t colorIndex = lmpd_map<uint8_t, uint8_t>(y, 0, rows - 1, 0, 255);

        const auto& ledColor = colors::from_palette<false, uint8_t>(colorIndex, palette);
        ctx.lamp.setPixelColorXY(x, rows - y, ledColor);
      }
      // set rest to black
      for (uint8_t y = mappedY; y <= rows; ++y)
        ctx.lamp.setPixelColorXY(x, rows - y, 0);
    }
  }

  static void on_enter_mode(auto& ctx) { ctx.state.soundEvent.reset(ctx); }

  struct StateTy
  {
    audio::SoundEventTy<> soundEvent;
  };
};

} // namespace modes::default_modes

#endif
