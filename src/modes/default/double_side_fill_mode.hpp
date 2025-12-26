#ifndef DOUBLE_SIDE_FILL_MODE_HPP
#define DOUBLE_SIDE_FILL_MODE_HPP

namespace modes::default_modes {

#include <cstdint>

struct DoubleSideFillMode : public modes::BasicMode
{
  static constexpr float randomVariation = 0.3;
  static constexpr uint32_t animationTiming = 250;

  struct StateTy
  {
    float progress;
    uint32_t color;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.progress = 1.0;
    ctx.state.color = 0;
  }

  static void loop(auto& ctx)
  {
    static constexpr float iteration = ctx.lamp.frameDurationMs / static_cast<float>(animationTiming);
    ctx.state.progress += iteration;

    const float prog = min<float>(ctx.state.progress, 1.0);

    const size_t endIndex = prog * (ctx.lamp.ledCount / 2.0);
    ctx.lamp.fill(ctx.state.color, 0, endIndex);

    const size_t startIndex = ctx.lamp.ledCount / 2.0 + (1.0 - prog) * ctx.lamp.ledCount / 2.0;
    ctx.lamp.fill(ctx.state.color, startIndex, ctx.lamp.ledCount);

    if (ctx.state.progress >= 1.0)
    {
      ctx.state.progress = 0.0;
      ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
    }
  }
};

} // namespace modes::default_modes

#endif
