#ifndef MODES_BLUETOOTH_CONTROLFIXEDMODES_H
#define MODES_BLUETOOTH_CONTROLFIXEDMODES_H

/// @file control_fixed_modes.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/palettes.hpp"
#include <cstdint>

namespace lampda::modes {
/// Define bluetooth special modes
namespace bluetooth {

struct ColorControlMode : public BasicMode
{
  struct StateTy
  {
    // color to display
    uint32_t color = 0xFFFFFF;
  };

  static void loop(auto& ctx)
  {
    //
    ctx.lamp.fill(ctx.state.color);
  }
};

namespace __private {

uint32_t get_color_at(size_t index)
{
  std::ignore = index;
  assert(false and "Invalid color index reached");
  return 0;
}

template<typename Head, typename... Tail> uint32_t get_color_at(size_t index, Head head, Tail... tail)
{
  if (index == 0)
    return head;
  return get_color_at(index - 1, tail...);
}
} // namespace __private

template<bool fastMode, uint8_t fadeRateUint, uint32_t... fadeColors> struct ColorSequenceMode : public BasicMode
{
  static constexpr float fadeRate = fadeRateUint / (2.0 * UINT8_MAX); // limit to range 0-0.5
  static constexpr size_t colorCount = sizeof...(fadeColors);
  static_assert((fadeRate == 0 and colorCount != 0) or (fadeRate > 0 and colorCount >= 2),
                "Cannot fade colors without at least two colors");

  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  struct StateTy
  {
    uint32_t colorStartTime_ms;
    uint8_t colorIndex;
  };

  static uint32_t next_color_index(auto& ctx)
  {
    const uint32_t nextIndex = ctx.state.colorIndex + 1;
    if (nextIndex >= colorCount)
      return 0;
    return nextIndex;
  }

  static void on_enter_mode(auto& ctx)
  {
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);

    ctx.state.colorStartTime_ms = ctx.lamp.now;
    ctx.state.colorIndex = 0;
  }

  static void loop(auto& ctx)
  {
    // ramp controls the speed
    static constexpr uint32_t lowestTiming = fastMode ? 100 : 1000;
    static constexpr uint32_t highestTiming = fastMode ? 15 : 100;
    // map ramp to period
    const uint32_t wholePaletteLoopTiming =
            lmpd_map<float>(ctx.get_active_custom_ramp(), 0, 255, lowestTiming, highestTiming);

    uint32_t timeSinceStart = ctx.lamp.now - ctx.state.colorStartTime_ms;
    if (timeSinceStart >= wholePaletteLoopTiming)
    {
      ctx.state.colorStartTime_ms = ctx.lamp.now;
      ctx.state.colorIndex = next_color_index(ctx);
      // Update time to prevent flashes
      timeSinceStart = ctx.lamp.now - ctx.state.colorStartTime_ms;
    }

    uint32_t color = __private::get_color_at(ctx.state.colorIndex, fadeColors...);

    if constexpr (fadeRateUint != 0)
    {
      const uint32_t fixedTime_ms = wholePaletteLoopTiming * (1.0 - fadeRate);
      if (timeSinceStart >= fixedTime_ms)
      {
        const uint32_t fadeTime_ms = wholePaletteLoopTiming * fadeRate;
        const float fadeProgress =
                lmpd_constrain((timeSinceStart - fixedTime_ms) / static_cast<float>(fadeTime_ms), 0.0f, 0.95f);
        const uint32_t colorEnd = __private::get_color_at(next_color_index(ctx), fadeColors...);
        color = utils::get_gradient(color, colorEnd, fadeProgress);
      }
    }

    ctx.lamp.fill(color);
  }
};

/// fixed modes
template<uint32_t color> struct FixedColorMode : public ColorSequenceMode<false, 0, color>
{
};

/// jump color
template<uint32_t... Colors> struct JumpColorMode : public ColorSequenceMode<false, 0, Colors...>
{
};

// fade mode
template<uint8_t fadeRate, uint32_t... Colors> struct FadeColorMode :
  public ColorSequenceMode<false, fadeRate, Colors...>
{
};

// flash mode
template<uint32_t... Colors> struct FlashColorMode : public ColorSequenceMode<true, 0, Colors...>
{
};

} // namespace bluetooth
} // namespace lampda::modes

#endif
