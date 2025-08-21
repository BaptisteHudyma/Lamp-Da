#ifndef AUTOMATON_MODE_HPP
#define AUTOMATON_MODE_HPP

#include "src/modes/include/draw/grid_rule.hpp"

#include "src/system/ext/random8.h"

/// Modes implementing cellular automaton using "grid" tools
namespace modes::automaton {

struct BubbleMode : public BasicMode
{
  using GridTy = draw::grid::LineRule<>;

  struct StateTy
  {
    GridTy grid;
    uint32_t algeaStart, algeaLifetime, algeaPos;
  };

  static constexpr int nbBubbles = 4;      // 0-10 max number of bubble per turn
  static constexpr int bubbleFreq = 70;    // 0-255 the more the less bubbles
  static constexpr int minBubbleDist = 4;  // 0-5 min distance of bubbles?
  static constexpr int eventFreq = 70;     // 0-255 how likely are events?
  static constexpr int algeaFreq = 60;     // 0-255 how likely event is algea?
  static constexpr int algeaLength = 3000; // (ms) +/- 1s average algea size
  static constexpr int algeaSwims = 400;   // (ms) how fast algea ondulates
  static constexpr int starFreq = 3;       // 0-255 how likely event is star?

  // colors
  static constexpr auto waterColor = colors::DarkBlue;
  static constexpr auto algeaColor = colors::ForestGreen;
  static constexpr auto starColor = colors::Gold;

  static void reset(auto& ctx)
  {
    // fill the initial automata with zeros +few colored pixels
    GridTy::LineTy first {};
    first.fill(waterColor);

    // reset grid object & config
    ctx.state.grid.reset(ctx.lamp, first);

    // reset stateful events
    ctx.state.algeaStart = 0;
  }

  static void loop(auto& ctx)
  {
    // rng helper
    uint8_t mini = ctx.get_active_custom_ramp() / 4;
    uint8_t maxi = 255;
    auto rng = [&](auto p) LMBD_INLINE {
      return random8(mini, maxi) < p;
    };

    // st & lamp for brievty
    auto& st = ctx.state;
    auto& lamp = ctx.lamp;

    // callback to update the automaton
    auto cb = [&](const auto& before, auto& after) {
      after.fill(waterColor);

      // add randomly bubbles
      for (size_t I = 0; I < nbBubbles; ++I)
      {
        if (rng(bubbleFreq))
          after[random8(0, after.size())] = colors::fromGrey(0xff);
      }

      // one pass to remove too crowded stuff
      int lastBubble = 255;
      for (int I = 0; I < after.size(); ++I)
      {
        // if something above, cancel bubble
        if (before[I] != waterColor)
          lastBubble = I;

        if (after[I] == waterColor)
          continue;

        // if other bubble too close, cancel bubble
        if (abs(int(lastBubble - I)) < minBubbleDist)
        {
          after[I] = waterColor;
        }

        lastBubble = I;
      }

      // trigger events
      if (rng(eventFreq) && rng(eventFreq))
      {
        if (rng(starFreq))
        {
          after[random8(0, after.size())] = starColor;
        }

        if (st.algeaStart == 0 && rng(algeaFreq))
        {
          st.algeaPos = random8(2, after.size() - 2);
          st.algeaStart = lamp.now;
          st.algeaLifetime = random16(algeaLength - 1000, algeaLength + 1000);
        }
      }

      // draw oscillating algea if active
      if (st.algeaStart && st.algeaStart + st.algeaLifetime > lamp.now)
      {
        int shift = abs(int(((lamp.now / algeaSwims) % 8) - 3)) - 2; // oscillate -2..+2
        after[st.algeaPos + shift] = algeaColor;
      }
      else
      {
        st.algeaStart = 0;
      }
    };

    // default grid loop
    ctx.state.grid.template loop<false>(ctx, cb);
  }

  static constexpr bool hasCustomRamp = true;
};

struct SierpinskiMode : public BasicMode
{
  struct ConfigTy : public draw::grid::LineRuleConfig
  {
    // scroll the grid skewed (no "bubble" effect
    static constexpr bool scrollSkewed = true;
    // add more blur
    static constexpr uint8_t renderBlurAmount = 128;
  };
  using GridTy = draw::grid::LineRule<ConfigTy>;

  struct StateTy
  {
    GridTy grid;
  };

  static void reset(auto& ctx)
  {
    // fill the initial automata with zeros +few colored pixels
    GridTy::LineTy first {};
    first[4] = colors::fromRGB(0x00, 0x00, 0x12);
    first[5] = colors::fromRGB(0x00, 0x00, 0x23);
    first[6] = colors::fromRGB(0x00, 0x20, 0x40);
    first[7] = colors::fromRGB(0x80, 0x80, 0x80);
    first[8] = colors::fromRGB(0x40, 0x40, 0x00);
    first[9] = colors::fromRGB(0x23, 0x0c, 0x00);
    first[10] = colors::fromRGB(0x12, 0x03, 0x00);

    // reset grid object & config
    ctx.state.grid.reset(ctx.lamp, first);
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    // setup a callback, use wolframRule's 1-d cellular automata
    auto cb = [&](const auto& before, auto& after) {
      draw::grid::wolframRule<90>(before, after);
    };

    // default grid loop
    ctx.state.grid.template loop<hasCustomRamp>(ctx, cb);
  }

  static constexpr bool hasCustomRamp = true;
};

using AutomatonModes = modes::GroupFor<BubbleMode, SierpinskiMode>;

} // namespace modes::automaton

#endif
