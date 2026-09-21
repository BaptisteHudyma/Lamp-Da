#ifndef WAVEFORM_MODE_H
#define WAVEFORM_MODE_H

/// @file waveform
#include <limits>

#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/audio/utils.hpp"

#include "src/system/component/sound.h"

namespace lampda::modes::default_modes {

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

    static constexpr size_t dataRead = state.soundEvent._dataLenght;
    const auto& soundData = state.soundEvent.dataAutoGained;
    const float noiseLevelScale = lmpd_constrain<float>(lmpd_map<float>(state.soundEvent.level,
                                                                        component::microphone::silenceLevelDb,
                                                                        component::microphone::highLevelDb,
                                                                        0.0f,
                                                                        1.0f),
                                                        0.0f,
                                                        1.0f);

    ctx.lamp.clear();
    for (int x = 0; x <= cols; x++)
    {
      const uint16_t mappedX = lmpd_map<uint16_t>(x, 0, cols, 0, (dataRead - 1) / 4);
      const uint16_t mappedY = lmpd_constrain<uint16_t>(lmpd_map<uint16_t>(soundData[mappedX] * noiseLevelScale,
                                                                           std::numeric_limits<int16_t>::min(),
                                                                           std::numeric_limits<int16_t>::max(),
                                                                           0,
                                                                           rows),
                                                        0,
                                                        rows);

      for (int y = 0; y < rows; y++)
      {
        if (((y >= rows / 2) && (y <= mappedY)) || ((y < rows / 2) && (y >= mappedY)))
        {
          const float colorLevel = lmpd_map<float>(y, 0, rows, -1.f, 1.f);
          const uint32_t ledColor = utils::get_gradient(
                  utils::ColorSpace::GREEN.get_rgb().color, utils::ColorSpace::RED.get_rgb().color, abs(colorLevel));
          ctx.lamp.setPixelColorXY(x, rows - y, ledColor);
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
    /// handle sound events
    audio::SoundEventTy<> soundEvent;
  };
};

} // namespace lampda::modes::default_modes

#endif // WAVEFORM_MODE_H
