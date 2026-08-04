#include "src/modes/include/draw/text_display.hpp"
#include "src/modes/include/navigation_request.hpp"


namespace lampda::modes::cli {

struct TextMode : public BasicMode
{
  static constexpr bool hasCustomRamp = true;

  static constexpr uint8_t min_period = 70;
  static constexpr uint8_t max_period = 250;

  struct StateTy
  {
    std::string text="";
    int16_t x = 0;
    uint32_t lastMovementMs = 0;
    uint32_t color = 0xFFFFFF;
    uint8_t nb_loop = 1;
    bool infinite_loop = false;

    uint8_t returnGroup = 0;
    bool hasReturnGroup = false;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.lamp.clear();
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    ctx.lamp.clear();
    const uint8_t index = ctx.get_active_custom_ramp();

    const uint32_t movementPeriodMs = min_period + ((index*(max_period-min_period))>>8);

    if (ctx.lamp.now - state.lastMovementMs >=
        movementPeriodMs)
    {
      const uint32_t elapsed =
          ctx.lamp.now - state.lastMovementMs;

      const uint32_t stepCount =
          elapsed / movementPeriodMs;

      state.x -= stepCount;
      state.lastMovementMs +=
          stepCount * movementPeriodMs;
    }

    constexpr float scale = 0.5f;
    constexpr int16_t fontHeight = 12;

    const int16_t y =
        (ctx.lamp.maxHeight - fontHeight) / 2;

    bool finished =
        draw::text::TextDisplay::display(
            ctx.lamp,
            state.text,
            [&state](char) -> uint32_t {
              return state.color;
            },
            state.x,
            y,
            scale,
            true);
    
    if( finished && state.nb_loop > 1)
    {
      finished = false;
      state.nb_loop--;
      state.x = ctx.lamp.maxWidth;
    }
    if( finished && state.infinite_loop)
    {
      finished = false;
      state.x = ctx.lamp.maxWidth;
    }

    if (finished && state.hasReturnGroup)
    {
      const uint8_t returnGroup =
          state.returnGroup;

      state.hasReturnGroup = false;
      state.text.clear();

      modes::navigation::request_group_change(
          returnGroup);

      return;
    }
    
    // if text is empty (lamp start on this mode), leave the mode
    if(state.text == "")
    {
      modes::navigation::request_group_change(0);
    }
  }
};

using CliModes = GroupFor<TextMode>;

} // namespace lampda::modes::cli