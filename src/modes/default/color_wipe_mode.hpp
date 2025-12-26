#ifndef COLOR_WIPE_MODE_HPP
#define COLOR_WIPE_MODE_HPP

namespace modes::default_modes {

#include <cstdint>

struct ColorWipeMode : public modes::BasicMode
{
  static constexpr float randomVariation = 0.3;
  static constexpr uint32_t animationTiming = 500;

  struct StateTy
  {
    float progress;
    bool step;
    uint32_t color;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.step = true;
    ctx.state.progress = 0.0;
    ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
  }

  static void loop(auto& ctx)
  {
    static constexpr float iteration = ctx.lamp.frameDurationMs / static_cast<float>(animationTiming);

    ctx.state.progress += iteration;
    const float prog = min<float>(ctx.state.progress, 1.0);

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

    if (ctx.state.progress >= 1.0)
    {
      ctx.state.progress = 0.0;
      ctx.state.step = not ctx.state.step;
      ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
    }
  }
};

} // namespace modes::default_modes

#endif
