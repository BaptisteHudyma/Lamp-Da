#ifndef VUMETER_MODE_H
#define VUMETER_MODE_H

/// @file vumeter
#include "src/modes/include/colors/palettes.hpp"

#include "src/modes/include/audio/utils.hpp"

namespace modes::default_modes {

/// Emulate a vu-meter
struct VuMeterMode : public BasicMode
{
  static constexpr auto palette = colors::PaletteGradient<colors::Red, colors::Green>;

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    state.soundEvent.update(ctx);

    const float decibels = state.soundEvent.level;

    ctx.lamp.fadeToBlackBy(state.fade);
    const uint16_t maxLedIndex = ctx.lamp.ledCount - 1;

    // convert the sound level in the height lamp level
    const uint16_t vuLevel = lmpd_constrain<uint16_t>(
            lmpd_map<uint16_t>(decibels, microphone::silenceLevelDb, microphone::highLevelDb, 0, maxLedIndex),
            ceilf(2.0 * ctx.lamp.maxWidthFloat),
            maxLedIndex);

    ctx.lamp.fill(palette, maxLedIndex - vuLevel, maxLedIndex);
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.fade = 128;
    ctx.state.soundEvent.reset(ctx);
  }

  struct StateTy
  {
    uint8_t fade;
    audio::SoundEventTy<> soundEvent;
  };
};

} // namespace modes::default_modes

#endif // VUMETER_MODE_H
