#ifndef WAVEFORM_MODE_H
#define WAVEFORM_MODE_H

/// @file waveform
#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/audio/utils.hpp"

namespace modes::default_modes {

/// Display sound waveforms from microphone samples
struct WaveformMode : public BasicMode
{
  static constexpr bool hasCustomRamp = true;
  static constexpr auto palette = colors::PaletteGradient<colors::Green, colors::Red>;
  static constexpr float maxLevelDb_threshold = microphone::highLevelDb * 0.8f;

  static void loop(auto& ctx)
  {
    const uint16_t cols = ctx.lamp.maxWidth - 1;
    const uint16_t rows = ctx.lamp.maxHeight - 1;
    auto& state = ctx.state;
    state.soundEvent.update(ctx);

    constexpr uint32_t ledColorBlack = 0x00;
    uint32_t ledColor = ledColorBlack;

    microphone::PdmData sound_data = ctx.lamp.get_sound_data();
    for (int x = 0; x < cols; x++)
    {
      float sampleDb = microphone::silenceLevelDb;
      if (sound_data.is_valid())
      {
        const uint8_t mappedX = lmpd_map<uint8_t, uint8_t>(x, 0, cols, 0, sound_data.sampleRead - 1);
        sampleDb = 20.0 * log10f(sound_data.data[mappedX] / 1024.f);
        sampleDb = lmpd_constrain(sampleDb, microphone::silenceLevelDb, microphone::highLevelDb);
      }
      const uint8_t mappedY = lmpd_constrain(
              lmpd_map<float, uint8_t>(sampleDb, microphone::silenceLevelDb, microphone::highLevelDb, 0, rows / 2),
              0,
              rows / 2);
      for (int y = 0; y < rows; y++)
      {
        if (y < mappedY)
        {
          ledColor = utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                          utils::ColorSpace::GREEN.get_rgb().color,
                                          abs(((float)rows / 2.f - y) / ((float)rows / 2.f)));
          ctx.lamp.setPixelColorXY(x, rows / 2 - y, ledColor);
          ctx.lamp.setPixelColorXY(x, rows / 2 + y, ledColor);
        }
        else
        {
          ctx.lamp.setPixelColorXY(x, rows / 2 - y, ledColorBlack);
          ctx.lamp.setPixelColorXY(x, rows / 2 + y, ledColorBlack);
        }
      }
    }
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.soundEvent.reset(ctx);
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  struct StateTy
  {
    audio::SoundEventTy<> soundEvent;
  };
};

} // namespace modes::default_modes

#endif // WAVEFORM_MODE_H
