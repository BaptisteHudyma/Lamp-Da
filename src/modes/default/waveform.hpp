#ifndef WAVEFORM_MODE_H
#define WAVEFORM_MODE_H

/// @file waveform
#include <limits>

#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/audio/utils.hpp"

namespace modes::default_modes {

/// Display sound waveforms from microphone samples
struct WaveformMode : public BasicMode
{
  static constexpr bool hasCustomRamp = true;
  static constexpr auto palette = colors::PaletteGradient<colors::Green, colors::Red>;

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
      int16_t sampleVolume = 0.f;
      if (sound_data.is_valid())
      {
        const uint8_t mappedX = lmpd_map<uint8_t, uint8_t>(x, 0, cols, 0, sound_data.sampleRead - 1);
        sampleVolume = sound_data.data[mappedX];
      }
      const uint8_t mappedY = lmpd_constrain(
              lmpd_map<int16_t, uint8_t>(
                      sampleVolume, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), 0, rows),
              0,
              rows);

      for (int y = 0; y < rows; y++)
      {
        if (((y >= rows / 2) && (y <= mappedY)) || ((y < rows / 2) && (y >= mappedY)))
        {
          float colorLevel = lmpd_map<int16_t, float>(y, 0, rows, -1.f, 1.f);
          ledColor = utils::get_gradient(
                  utils::ColorSpace::GREEN.get_rgb().color, utils::ColorSpace::RED.get_rgb().color, abs(colorLevel));
          ctx.lamp.setPixelColorXY(x, rows - y, ledColor);
        }
        else
        {
          ctx.lamp.setPixelColorXY(x, rows - y, ledColorBlack);
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
