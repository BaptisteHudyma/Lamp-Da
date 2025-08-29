#ifndef BRIGHTNESS_MODES_H
#define BRIGHTNESS_MODES_H

namespace modes::brightness {
/// Basic lightning mode (does nothing, brightness may be adjusted)
struct StaticLightMode : public BasicMode
{
  static constexpr void loop(auto& ctx)
  {
    if constexpr (ctx.lamp.flavor == hardware::LampTypes::indexable)
      ctx.lamp.fill(colors::Candle);
  }
};

/// One mode, alone in its group, for static lightning
using StaticLightOnly = GroupFor<StaticLightMode>;

/// Simple pulse every second
struct OnePulse : public BasicMode
{
  static constexpr uint32_t periodMs = 1000;
  static constexpr uint32_t pulseSizeMs = 100;
  static constexpr brightness_t minScaling = 50;
  static constexpr float scaleFactor = 1.40;

  static constexpr void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    brightness_t base = lamp.getSavedBrightness();
    brightness_t scaled = max(base * scaleFactor, base + minScaling);

    if (scaled > lamp.maxBrightness)
      lamp.jumpBrightness(lamp.maxBrightness / scaleFactor);

    auto period = lamp.now % periodMs;
    if (period < pulseSizeMs)
      lamp.tempBrightness(scaled);
    else
      lamp.restoreBrightness();
  }
};

/// Pulse N times then pause one second
template<size_t N> struct ManyPulse : public BasicMode
{
  static constexpr uint32_t pulseSizeMs = 100;
  static constexpr uint32_t periodMs = 1000 + N * pulseSizeMs * 2;
  static constexpr brightness_t minScaling = 50;
  static constexpr float scaleFactor = 1.40;

  static constexpr void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    brightness_t base = lamp.getSavedBrightness();
    brightness_t scaled = max(base * scaleFactor, base + minScaling);

    if (scaled > lamp.maxBrightness)
      lamp.jumpBrightness(lamp.maxBrightness / scaleFactor);

    auto period = lamp.now % periodMs;
    if (period < pulseSizeMs * N * 2)
    {
      uint32_t flipflop = period / pulseSizeMs;
      if (flipflop % 2)
        lamp.restoreBrightness();
      else
        lamp.tempBrightness(scaled);
    }
    else
      lamp.restoreBrightness();
  }
};

using FlashesGroup = GroupFor<OnePulse, ManyPulse<2>, ManyPulse<3>>;

} // namespace modes::brightness

#endif
