#ifndef VUMETER_MODE_H
#define VUMETER_MODE_H

/// @file vumeter
#include "src/modes/include/colors/palettes.hpp"

namespace modes::default_modes {

/// Emulate a vu-meter
struct VuMeterMode : public BasicMode
{
  static constexpr auto palette = colors::PaletteGradient<colors::Green, colors::Red>;

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    ctx.soundEvent.update(ctx);

    const float decibels = ctx.soundEvent.level;

    ctx.lamp.fadeToBlackBy(state.fade);

    // convert the sound level in the height lamp level
    const uint16_t vuLevel = lmpd_constrain<uint16_t>(
            lmpd_map<float>(decibels, microphone::silenceLevelDb, microphone::highLevelDb, 0, ctx.lamp.ledCount - 1),
            ceilf(ctx.lamp.maxWidthFloat),
            ctx.lamp.ledCount - 1);

    ctx.lamp.fill(palette, 0, vuLevel);
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.fade = 128;
    ctx.soundEvent.reset(ctx);
  }

  struct StateTy
  {
    uint8_t fade;
  };
};

} // namespace modes::default_modes

#endif // VUMETER_MODE_H
