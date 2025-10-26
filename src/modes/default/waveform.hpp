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
    ctx.soundEvent.update(ctx);

    constexpr uint32_t ledColorBlack = 0x00;
    uint32_t ledColor = ledColorBlack;

    static constexpr size_t dataRead = ctx.soundEvent._dataLenght;
    const auto& soundData = ctx.soundEvent.data;

    ctx.lamp.clear();
    for (int x = 0; x < cols; x++)
    {
      const uint16_t mappedX = lmpd_map<uint16_t>(x, 0, cols, 0, dataRead - 1);
      const uint16_t mappedY = lmpd_constrain<uint16_t>(lmpd_map<uint16_t>(soundData[mappedX],
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
          float colorLevel = lmpd_map<float>(y, 0, rows, -1.f, 1.f);
          ledColor = utils::get_gradient(
                  utils::ColorSpace::GREEN.get_rgb().color, utils::ColorSpace::RED.get_rgb().color, abs(colorLevel));
          ctx.lamp.setPixelColorXY(x, rows - y, ledColor);
        }
      }
    }
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.soundEvent.reset(ctx);
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  struct StateTy
  {
  };
};

} // namespace modes::default_modes

#endif // WAVEFORM_MODE_H
