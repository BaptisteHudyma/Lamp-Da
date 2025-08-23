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
  static constexpr float maxLevelDb_threshold = microphone::highLevelDb * 0.8;

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    ctx.state.soundEvent.update(ctx);

    const float decibels = ctx.state.soundEvent.level;

    // measure custom ramp for fire sound sensitivity
    const float index = ctx.get_active_custom_ramp();
    // compute the threshold
    const float threshold_db = microphone::silenceLevelDb + index * (maxLevelDb_threshold) / 255;

    // convert the sound level into 0 - 1
    // 0 = no led light up
    // 1 = all led light up
    const float vuLevel = (decibels + abs(threshold_db)) / microphone::highLevelDb;

    ctx.lamp.fill(palette, (decibels > threshold_db) ? vuLevel : 0);
  }

  static void reset(auto& ctx)
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

#endif // VUMETER_MODE_H
