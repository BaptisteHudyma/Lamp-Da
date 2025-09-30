

namespace modes::nudz
{

//   struct ImageTy {
//     static constexpr uint16_t width = 25;
//     static constexpr uint16_t height = 22;
//     static constexpr uint32_t rgbData[] = { 0x800000, 0x008000 };
//   };

  template <typename ImageType>
  struct NudzScrollImageMode : public BasicMode {

    struct StateTy
    {
      float minSpeed = -0.5f;
      float maxSpeed = 0.5f;
      uint32_t decal;
      uint32_t last_tick;
    };

    static void on_enter_mode(auto& ctx)
    {
      // reset stateful events
      ctx.state.decal = 0;
      ctx.state.last_tick = 0;
    }

    static void loop(auto& ctx) {
      // ctx.lamp.fill(colors::Green);
      // ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);

      float spdRange = ctx.state.maxSpeed - ctx.state.minSpeed;
      float speed = ctx.state.minSpeed
        + (float(ctx.get_active_custom_ramp()) / 255) * spdRange;
      // ease stop at 0
      if(speed < -spdRange * 0.1)
        speed += spdRange * 0.1;
      else if(speed > spdRange * 0.1)
        speed -= spdRange * 0.1;
      else
        speed = 0.f;

      // decal needs a state to keep continuous while speed is changed
      int32_t decal = ctx.state.decal
        + int32_t((ctx.lamp.tick - ctx.state.last_tick) * speed);
      if(decal != ctx.state.decal)
      {
        ctx.state.decal = decal;
        ctx.state.last_tick = ctx.lamp.tick;
      }

      uint16_t imWidth = ImageType::width;
      uint16_t imHeight = ImageType::height;
      uint32_t w = std::min(uint32_t(ctx.lamp.maxWidth), uint32_t(imWidth));
      uint32_t h = std::min(uint32_t(ctx.lamp.maxHeight), uint32_t(imHeight));
      // const uint32_t *simage = image();
      for(uint32_t y=0; y<h; ++y)
        for(uint32_t x=0; x<w; ++x)
          ctx.lamp.setPixelColorXY(x, y,
                                   ImageType::rgbData[y * imWidth
                                   + (x + decal) % imWidth]);
    }

    static constexpr bool hasCustomRamp = true;
  };


#include "heineken_image.hpp"

  typedef NudzScrollImageMode<HeinekenImageTy> NudzHeinekenMode;

#include "violonsaouls_image.hpp"

  struct NudzViolonsaoulsMode : public NudzScrollImageMode<ViolonsaoulsImageTy>
  {
    struct StateTy
    {
      float minSpeed = 0.f;
      float maxSpeed = 0.5f;
      uint32_t decal;
      uint32_t last_tick;
    };

  };

}
