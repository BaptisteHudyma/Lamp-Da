

namespace modes::nudz
{

  struct NudzScrollImageMode : public BasicMode {

    struct StateTy
    {
      uint32_t decal;
      uint32_t last_tick;
    };

    static void on_enter_mode(auto& ctx)
    {
      // reset stateful events
      ctx.state.decal = 0;
      ctx.state.last_tick = 0;
    }

    static void loop_image(auto& ctx, const uint32_t *simage) {
      // ctx.lamp.fill(colors::Green);
      // ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);

      float speed = (float(ctx.get_active_custom_ramp()) - 128) / 500.f;
      // ease stop at 0
      if(speed < -0.01)
        speed += 0.01;
      else if(speed > 0.01)
        speed -= 0.01;
      else
        speed = 0.f;

      // decal needs a state to keep continuous while speed is changed
      uint32_t decal = ctx.state.decal
        + unsigned((ctx.lamp.tick - ctx.state.last_tick) * speed);
      if(decal != ctx.state.decal)
      {
        ctx.state.decal = decal;
        ctx.state.last_tick = ctx.lamp.tick;
      }

      uint16_t imWidth = 25;
      uint16_t imHeight = 22;
      uint32_t w = std::min(uint32_t(ctx.lamp.maxWidth), uint32_t(imWidth));
      uint32_t h = std::min(uint32_t(ctx.lamp.maxHeight), uint32_t(imHeight));
      // const uint32_t *simage = image();
      for(uint32_t y=0; y<h; ++y)
        for(uint32_t x=0; x<w; ++x)
          ctx.lamp.setPixelColorXY(x, y,
                                   simage[y * imWidth
                                   + (x + decal) % imWidth]);
    }

    static constexpr bool hasCustomRamp = true;
  };


  struct NudzHeinekenMode : public NudzScrollImageMode {

#include "heineken_image.hpp"

    static void loop(auto& ctx) {
      loop_image(ctx, heineken_image);
    }

  };

}
