#ifndef PING_PONG_MODE_HPP
#define PING_PONG_MODE_HPP

namespace modes::default_modes {

#include <cstdint>

struct PingPongMode : public modes::BasicMode
{
  static constexpr float randomVariation = 0.3;
  static constexpr uint32_t animationTiming = 1000;

  struct StateTy
  {
    size_t lastIndex;
    float progress;
    bool step;
    uint32_t color;
    uint8_t persistance;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.lastIndex = 0;
    ctx.state.persistance = 128;
    ctx.state.progress = 0.0;
    ctx.state.step = true;
    ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
  }

  static void loop(auto& ctx)
  {
    static constexpr float iteration = ctx.lamp.frameDurationMs / static_cast<float>(animationTiming / 2.0f);
    static constexpr auto maxLedIndex = ctx.lamp.ledCount - 1;
    ctx.state.progress += iteration;

    const float prog = min<float>(ctx.state.progress, 1.0);

    // fade leds
    ctx.lamp.fadeToBlackBy(255 - ctx.state.persistance);

    // display new ramps
    if (ctx.state.step)
    {
      const size_t newIndex = prog * maxLedIndex;
      for (size_t i = ctx.state.lastIndex; i < newIndex; i++)
        ctx.lamp.setPixelColor(i, ctx.state.color);
      ctx.state.lastIndex = newIndex;
    }
    else
    {
      const size_t newIndex = max<float>(0.0, (1.0f - prog) * maxLedIndex);
      for (size_t i = newIndex; i < ctx.state.lastIndex; i++)
        ctx.lamp.setPixelColor(i, ctx.state.color);
      ctx.state.lastIndex = newIndex;
    }

    // end of cycle
    if (ctx.state.progress >= 1.0)
    {
      ctx.state.progress = 0.0;
      ctx.state.step = not ctx.state.step;

      if (ctx.state.step)
        ctx.state.color = utils::get_random_complementary_color(ctx.state.color, randomVariation);
    }
  }
};

} // namespace modes::default_modes

#endif
