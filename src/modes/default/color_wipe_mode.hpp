#ifndef COLOR_WIPE_MODE_HPP
#define COLOR_WIPE_MODE_HPP

/// @file color_wipe_mode.hpp

namespace lampda::modes::default_modes {

#include <cstdint>

/**
 * \brief Wipe a color from one side to the other, with random color variations
 */
struct ColorWipeMode : public modes::BasicMode
{
  /// Random variation of the color between animation loops
  static constexpr float randomVariation = 0.3f;
  /// Lenght of the animation, in milliseconds
  static constexpr uint32_t animationTiming = 500;

  struct StateTy
  {
    /// Between 0 and 1, progress
    float progress;
    /// true wipe up, false wipe down
    bool step;
    /// actual color to display
    uint32_t color;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.step = true;
    ctx.state.progress = 0.0f;
    ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
  }

  static void loop(auto& ctx)
  {
    static constexpr float iteration = ctx.lamp.frameDurationMs / static_cast<float>(animationTiming);

    ctx.state.progress += iteration;
    const float prog = std::min<float>(ctx.state.progress, 1.0f);

    // go up
    if (ctx.state.step)
    {
      const size_t endIndex = prog * ctx.lamp.ledCount;
      ctx.lamp.fill(ctx.state.color, 0, endIndex);
    }
    // go down
    else
    {
      const size_t startIndex = (1.0f - prog) * ctx.lamp.ledCount;
      ctx.lamp.fill(ctx.state.color, startIndex, ctx.lamp.ledCount);
    }

    if (ctx.state.progress >= 1.0f)
    {
      ctx.state.progress = 0.0f;
      ctx.state.step = not ctx.state.step;
      ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
    }
  }
};

} // namespace lampda::modes::default_modes

#endif
