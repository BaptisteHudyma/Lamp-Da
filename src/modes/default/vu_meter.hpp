#ifndef VUMETER_MODE_H
#define VUMETER_MODE_H

/// @file vumeter
#include "src/modes/include/colors/palettes.hpp"
#include "src/modes/include/audio/utils.hpp"

namespace modes::default_modes {

/// Emulate a vu-meter, optionally sound-sensitive
struct VuMeterMode : public BasicMode
{
  static constexpr bool hasCustomRamp = true;
  static constexpr auto palette = colors::PaletteGradient<colors::Green, colors::Red>;
  static constexpr float maxLevelDb_threshold = microphone::highLevelDb * 0.8f;

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    ctx.state.soundEvent.update(ctx);

    const float decibels = ctx.state.soundEvent.level;

    ctx.lamp.fadeToBlackBy(state.fade);

    // convert the sound level in the height lamp level
    const uint16_t vuLevel = lmpd_constrain(
            lmpd_map<float, float>(
                    decibels, microphone::silenceLevelDb, microphone::highLevelDb, 0, ctx.lamp.ledCount - 1),
            0,
            ctx.lamp.ledCount - 1);

    ctx.lamp.fill(palette, 0, vuLevel);
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.fade = 128;
    ctx.state.soundEvent.reset(ctx);
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  struct StateTy
  {
    audio::SoundEventTy<> soundEvent;
    uint8_t fade;
  };
};

} // namespace modes::default_modes

#endif // VUMETER_MODE_H
