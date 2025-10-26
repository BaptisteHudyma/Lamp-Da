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

  static void loop(auto& ctx)
  {
    const uint16_t cols = ctx.lamp.maxWidth - 1;
    const uint16_t rows = ctx.lamp.maxHeight - 1;
    auto& state = ctx.state;
    ctx.soundEvent.update(ctx);
    state.currentColumn = (state.currentColumn + 1) % cols;

    constexpr uint32_t ledColorBlack = 0x00;
    uint32_t ledColor = ledColorBlack;

    const float decibels = ctx.soundEvent.level;

    const float soundLevel = lmpd_constrain<float>(
            lmpd_map<float>(decibels, microphone::silenceLevelDb, microphone::highLevelDb, 0.f, 1.f), 0.f, 1.f);

    const uint8_t mappedY = lmpd_constrain<uint8_t>(lmpd_map<uint8_t>(soundLevel, 0.f, 1.f, 0, rows), 0, rows);
    ledColor = utils::get_gradient(
            utils::ColorSpace::GREEN.get_rgb().color, utils::ColorSpace::RED.get_rgb().color, soundLevel);

    for (int y = 0; y < rows; y++)
    {
      if (y == mappedY)
      {
        ctx.lamp.setPixelColorXY(state.currentColumn, rows - y, ledColor);
      }
      else
      {
        ctx.lamp.setPixelColorXY(state.currentColumn, rows - y, ledColorBlack);
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
    uint16_t currentColumn = 0;
  };
};

} // namespace modes::default_modes

#endif // WAVEFORM_MODE_H
