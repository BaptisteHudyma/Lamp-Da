
#include "src/system/physical/imu.h"

namespace modes::custom::nudz {

//   struct ImageTy {
//     static constexpr uint16_t width = 26;
//     static constexpr uint16_t height = 22;
//     static constexpr uint32_t colormapSize = 0;
//     static constexpr uint32_t colormap[];
//     static constexpr uint8_t indexData[] = {};
//     static constexpr uint32_t rgbData[] = { 0x800000, 0x008000 };
//   };

template<typename ImageType> struct NudzScrollImageMode : public BasicMode
{
  struct StateTy
  {
    float minSpeed = -0.5f;
    float maxSpeed = 0.5f;
    bool randomScroll = false;
    uint32_t xdecal;
    uint32_t ydecal;
    uint32_t last_tick;
    int8_t xdirection = 1;
    int8_t ydirection = 0;
  };

  static void on_enter_mode(auto& ctx)
  {
    // reset stateful events
    ctx.state.xdecal = 0;
    ctx.state.ydecal = 0;
    ctx.state.last_tick = 0;
  }

  static void loop(auto& ctx)
  {
    // ctx.lamp.fill(colors::Green);
    // ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);

    float spdRange = ctx.state.maxSpeed - ctx.state.minSpeed;
    float speed = ctx.state.minSpeed + (float(ctx.get_active_custom_ramp()) / 255) * spdRange;
    // ease stop at 0
    if (speed < -spdRange * 0.1)
      speed += spdRange * 0.1;
    else if (speed > spdRange * 0.1)
      speed -= spdRange * 0.1;
    else
      speed = 0.f;

    uint16_t imWidth = ImageType::width;
    uint16_t imHeight = ImageType::height;

    // decal needs a state to keep continuous while speed is changed
    int32_t xdecal = ctx.state.xdecal + int32_t((ctx.lamp.tick - ctx.state.last_tick) * speed);
    int32_t ydecal = 0;
    if (ctx.state.randomScroll)
    {
      xdecal =
              int32_t(ctx.state.xdecal) + int32_t((ctx.lamp.tick - ctx.state.last_tick) * speed) * ctx.state.xdirection;
      if (ctx.state.ydirection != 0)
        ydecal = int32_t(ctx.state.ydecal) +
                 int32_t((ctx.lamp.tick - ctx.state.last_tick) * speed) * ctx.state.ydirection;
      if (xdecal < 0)
      {
        xdecal = 0;
        ctx.state.xdirection *= -random8(2);
        ctx.state.ydirection = random8(3) - 1;
        if (ctx.state.xdirection == 0 && ctx.state.ydirection == 0)
          ctx.state.ydirection = random8(2) * 2 - 1;
      }
      else if (xdecal + ctx.lamp.maxWidth >= imWidth)
      {
        xdecal = imWidth - ctx.lamp.maxWidth - 1;
        ctx.state.xdirection *= -random8(2);
        ctx.state.ydirection = random8(3) - 1;
        if (ctx.state.xdirection == 0 && ctx.state.ydirection == 0)
          ctx.state.ydirection = random8(2) * 2 - 1;
      }
      if (ydecal < 0)
      {
        ydecal = 0;
        ctx.state.ydirection *= -random8(2);
        ctx.state.xdirection = random8(3) - 1;
        if (ctx.state.xdirection == 0 && ctx.state.ydirection == 0)
          ctx.state.xdirection = random8(2) * 2 - 1;
      }
      else if (ydecal + ctx.lamp.maxHeight >= imHeight)
      {
        ydecal = imHeight - ctx.lamp.maxHeight - 1;
        ctx.state.ydirection *= -random8(2);
        ctx.state.xdirection = random8(3) - 1;
        if (ctx.state.xdirection == 0 && ctx.state.ydirection == 0)
          ctx.state.xdirection = random8(2) * 2 - 1;
      }
    }
    if (xdecal != ctx.state.xdecal || ydecal != ctx.state.ydecal)
    {
      ctx.state.xdecal = xdecal;
      ctx.state.ydecal = ydecal;
      ctx.state.last_tick = ctx.lamp.tick;
    }

    uint32_t w = std::min(uint32_t(ctx.lamp.maxWidth + 1), uint32_t(imWidth));
    uint32_t h = std::min(uint32_t(ctx.lamp.maxHeight), uint32_t(imHeight));
    if (ImageType::colormapSize == 0)
      for (uint32_t y = 0; y < h; ++y)
        for (uint32_t x = 0; x < w; ++x)
          ctx.lamp.setPixelColorXY(
                  x, y, ImageType::rgbData[((y + ydecal) % imHeight) * imWidth + (x + xdecal) % imWidth]);
    else
    {
      // indexed colormap
      uint8_t bmask = (1 << ImageType::bitsPerPixel) - 1;
      for (uint32_t y = 0; y < h; ++y)
        for (uint32_t x = 0; x < w; ++x)
        {
          uint32_t offset = ((y + ydecal) % imHeight) * imWidth + (x + xdecal) % imWidth;
          uint32_t byteOffset = offset * ImageType::bitsPerPixel / 8;
          uint32_t bitOffset = (offset * ImageType::bitsPerPixel) % 8;
          uint8_t index = ImageType::indexData[byteOffset];
          if (bitOffset + ImageType::bitsPerPixel > 8)
          {
            uint16_t sindex = (index << 8) | ImageType::indexData[byteOffset + 1];
            index = (sindex >> (16 - bitOffset - ImageType::bitsPerPixel)) & bmask;
          }
          else
            index = (index >> (8 - bitOffset - ImageType::bitsPerPixel)) & bmask;

          ctx.lamp.setPixelColorXY(x, y, ImageType::colormap[index]);
        }
    }
  }

  static constexpr bool hasCustomRamp = true;
};

#include "heineken_image.hpp"

typedef NudzScrollImageMode<HeinekenImageTy> NudzHeinekenMode;

#include "huit_six_image.hpp"

typedef NudzScrollImageMode<Huit_sixImageTy> NudzHuitSixMode;

#include "violonsaouls_image.hpp"

struct NudzViolonsaoulsMode : public NudzScrollImageMode<ViolonsaoulsImageTy>
{
  struct StateTy
  {
    float minSpeed = 0.f;
    float maxSpeed = 0.5f;
    bool randomScroll = false;
    uint32_t xdecal;
    uint32_t ydecal;
    uint32_t last_tick;
    int8_t xdirection = 1;
    int8_t ydirection = 0;
  };
};

struct NudzBeerGlassMode : public BasicMode
{
  struct BubbleTy
  {
    BubbleTy() : x(-1.f), y(0.f), speed(0.f), color(0x706050), color_fade(0.9) {}
    int x;
    float y;
    float speed;
    uint32_t color;
    float color_fade;
  };

  struct StateTy
  {
    float level;        // average level of beer
    float ampl;         // accel amplitude factor
    float accmax;       // clamp accel to avoid too large changes
    float fall_ampl;    // height a wave will go down at each step
    float wave_ampl;    // wave speed
    float decay;        // speed decay factor
    float bounce_ratio; // if ratio > 0.5 a wave dropping can increae
                        // its neighbours height over its new height,
                        // thus produce a new wave
    uint32_t beer_color;
    uint32_t foam_color;
    uint32_t background_color;
    std::vector<float> levels;
    std::vector<float> speeds;
    uint32_t nbubbles;
    std::vector<BubbleTy> bubbles;
  };

  static void on_enter_mode(auto& ctx)
  {
    // reset stateful events
    ctx.state.level = 10.f;
    ctx.state.ampl = 0.02f;
    ctx.state.accmax = 0.2f;
    ctx.state.fall_ampl = 10.f;
    ctx.state.wave_ampl = 0.3f;
    ctx.state.decay = 0.7f;
    ctx.state.bounce_ratio = 0.7f;
    ctx.state.beer_color = 0x503000;
    ctx.state.foam_color = 0x706050;
    ctx.state.background_color = 0x000000;
    ctx.state.levels = std::vector<float>(ctx.lamp.maxWidth, ctx.state.level);
    ctx.state.speeds = std::vector<float>(ctx.lamp.maxWidth, 0.f);
    ctx.state.nbubbles = 20;
    ctx.state.bubbles = std::vector<BubbleTy>(ctx.state.nbubbles, BubbleTy());
    imu::get_filtered_reading(true);
  }

  static void loop(auto& ctx)
  {
    updateLevels(ctx);

    // display
    displayLevels(ctx);

    // bubbles
    makeBubbles(ctx);

    // draw accel
    if (ctx.get_active_custom_ramp() >= 240)
      drawAccel(ctx);
  }

  static void updateLevels(auto& ctx)
  {
    uint32_t nx = ctx.lamp.maxWidth;
    uint32_t ny = ctx.lamp.maxHeight;

    const auto& reading = imu::get_filtered_reading(false);
    const auto accel = reading.accel;

    auto& levels = ctx.state.levels;
    auto& speeds = ctx.state.speeds;

    // change global level, if needed, according to custom ramp
    float newLevel = float(ctx.get_active_custom_ramp()) / 255 * (ny - 1);
    float diffLevel = newLevel - ctx.state.level;
    if (diffLevel < 0)
    {
      // avoid leak in beer quantity
      float missing = 0.f;
      for (uint32_t x = 0; x < nx; ++x)
        if (levels[x] < -diffLevel)
          missing += -diffLevel - levels[x];
      diffLevel += missing / nx;
    }
    ctx.state.level = newLevel;
    for (uint32_t x = 0; x < nx; ++x)
      levels[x] += diffLevel;

    vec2d acc(accel.x, accel.y);
    // threshold because there is a drift in the accel
    if (acc.x * acc.x + acc.y * acc.y < 4.f)
      acc = vec2d(0.f, 0.f);
    acc.x *= ctx.state.ampl;
    acc.y *= ctx.state.ampl;
    float accmax = ctx.state.accmax;
    if (acc.x < -accmax)
      acc.x = -accmax;
    else if (acc.x > accmax)
      acc.x = accmax;
    if (acc.y < -accmax)
      acc.y = -accmax;
    else if (acc.y > accmax)
      acc.y = accmax;

    std::vector<float> ospeeds = speeds;

    // accelerate column by column
    // the "column" model is very simple and easy,
    // but cannot account for real orientation changes
    // since the Z (column) axis is always considered to be vertical
    for (uint32_t x = 0; x < nx; ++x)
    {
      // angular coords of the pixel
      float angle = float(x) / nx * M_PI * 2;
      // normal and tangent to the lamp
      vec2d norm(cos(angle), sin(angle));
      vec2d tan(-norm.y, norm.x);
      // we consider the accel will push pixels away from the wall
      // in the direction opposite to the force (if we consider the lamp
      // as the reference coords
      float acc0 = norm.dot(acc);
      if (acc0 < 0.f) // force and normal in the same direction:
        acc0 = 0.f;   // we cannot move because the lamp wall blocks it

      float sign = 1.f;
      if (acc.dot(tan) > 0) // direction of the propagation
        sign = -1.f;
      speeds[x] -= levels[x] * acc0 * sign;
      ospeeds[x] -= levels[x] * acc0 * sign;
      float diff = speeds[x]; // quantity of "beer pixels" which should
                              // transfer to neighboring column
      uint32_t x1, x2;
      if (diff < 0)
      {
        if (x == 0)
          x1 = nx - 1;
        else
          x1 = x - 1;
        if (levels[x] < -diff) // we cannot remove more material than
                               // the quantity available in this col.
          diff = -levels[x];
        levels[x1] -= diff; // transfer from col x to x1
        levels[x] += diff;
        ospeeds[x1] += diff * ctx.state.wave_ampl; // impulse a speed to x1
      }
      else
      {
        diff *= 0.7; // compensate loop propagation direction
                     // because the iteration is left-to-right
                     // and moves to the right are thus way faster
        if (x == nx - 1)
          x1 = 0;
        else
          x1 = x + 1;
        if (levels[x] < diff)
          diff = levels[x];
        levels[x1] += diff;
        levels[x] -= diff;
        ospeeds[x1] += diff * ctx.state.wave_ampl;
      }
    }

    // we have worked in a copy of speeds to avoid too fast propagation
    // in the iteration direction (left to right)
    ctx.state.speeds = ospeeds;

    // drop and move waves
    for (uint32_t x = 0; x < nx; ++x)
    {
      uint32_t x1 = x + 1;
      if (x == nx - 1)
        x1 = 0;
      float ld = levels[x1] - levels[x];
      float diff = 0;
      if (ld > 0)
        diff = std::min(ld, ctx.state.fall_ampl);
      else if (ld < 0)
        diff = std::max(ld, -ctx.state.fall_ampl);
      if (diff > 0 && diff > ld * ctx.state.bounce_ratio)
        diff = ld * ctx.state.bounce_ratio;
      else if (diff < 0 && diff < ld * ctx.state.bounce_ratio)
        diff = ld * ctx.state.bounce_ratio;
      levels[x] += diff;
      levels[x1] -= diff;
      speeds[x] -= diff * ctx.state.wave_ampl;
      speeds[x] *= ctx.state.decay;
    }
  }

  static void displayLevels(auto& ctx)
  {
    auto& levels = ctx.state.levels;
    uint32_t nx = ctx.lamp.maxWidth;
    uint32_t ny = ctx.lamp.maxHeight;

    for (uint32_t x = 0; x <= nx; ++x)
    {
      int32_t x0 = x;
      if (x == nx)
        x0 = nx - 1;
      int32_t y, y1 = int(levels[x0]);
      if (y1 >= ny)
        y1 = ny - 1;
      y1 = ny - 1 - y1;
      for (y = 0; y < y1 - 3; ++y)
        ctx.lamp.setPixelColorXY(x, y, ctx.state.background_color);
      for (; y < y1; ++y)
        ctx.lamp.setPixelColorXY(x, y, ctx.state.foam_color);
      for (; y < ny; ++y)
        ctx.lamp.setPixelColorXY(x, y, ctx.state.beer_color);
    }
  }

  static void makeBubbles(auto& ctx)
  {
    auto& bubbles = ctx.state.bubbles;
    auto& levels = ctx.state.levels;
    uint32_t nx = ctx.lamp.maxWidth;
    uint32_t ny = ctx.lamp.maxHeight;

    for (uint32_t i = 0; i < ctx.state.nbubbles; ++i)
    {
      auto& bubble = bubbles[i];
      if (bubble.x < 0) // free slot
      {
        // new bubble
        bubble.x = random8(nx);
        bubble.y = float(random8(uint8_t(levels[bubble.x])));
        bubble.speed = float(random8()) / 256 * 0.05 + 0.05;
        bubble.color = ctx.state.foam_color;
        bubble.color_fade = float(random8()) / 256 * 0.029 + 0.975;
      }
      if (int(bubble.y) >= levels[bubble.x])
      {
        bubble.x = -1; // die
        continue;
      }
      ctx.lamp.setPixelColorXY(bubble.x, ny - 1 - int(bubble.y), bubble.color);

      // move for next time
      bubble.y += bubble.speed;
      // fade color to beer color
      uint8_t r = uint8_t(float((bubble.color & 0xff0000) >> 16) * bubble.color_fade +
                          float((ctx.state.beer_color & 0xff0000) >> 16) * (1. - bubble.color_fade));
      uint8_t g = uint8_t(float((bubble.color & 0xff00) >> 8) * bubble.color_fade +
                          float((ctx.state.beer_color & 0xff00) >> 8) * (1. - bubble.color_fade));
      uint8_t b = uint8_t(float(bubble.color & 0xff) * bubble.color_fade +
                          float(ctx.state.beer_color & 0xff) * (1. - bubble.color_fade));
      bubble.color = (r << 16) + (g << 8) + b;
      // if we get cose to beer color, the bubble gets invisible and dies
      // theshold is 5 in R, G, B
      if ((float(r) - float((ctx.state.beer_color & 0xff0000) >> 16)) *
                          (float(r) - float((ctx.state.beer_color & 0xff0000) >> 16)) +
                  (float(g) - float((ctx.state.beer_color & 0xff00) >> 8)) *
                          (float(g) - float((ctx.state.beer_color & 0xff00) >> 8)) +
                  (float(b) - float(ctx.state.beer_color & 0xff)) * (float(b) - float(ctx.state.beer_color & 0xff)) <
          125)
        bubble.x = -1;
    }
  }

  static void drawAccel(auto& ctx)
  {
    const auto& reading = imu::get_filtered_reading(false);
    const auto accel = reading.accel;

    int32_t ax = int32_t(accel.x);
    int32_t ay = int32_t(accel.y);
    int32_t az = int32_t(accel.z);

    // clamp to -10 - 10
    if (ax < -10)
      ax = -10;
    else if (ax > 10)
      ax = 10;
    if (ay < -10)
      ay = -10;
    else if (ay > 10)
      ay = 10;
    if (az < -10)
      az = -10;
    else if (az > 10)
      az = 10;

    uint32_t x;
    for (x = 10 + ax; x < 10; ++x)
      ctx.lamp.setPixelColorXY(x, 0, 0x800000);
    for (x = 10; x < 10 + ax; ++x)
      ctx.lamp.setPixelColorXY(x, 0, 0x008000);
    for (x = 10 + ay; x < 10; ++x)
      ctx.lamp.setPixelColorXY(x, 1, 0x800000);
    for (x = 10; x < 10 + ay; ++x)
      ctx.lamp.setPixelColorXY(x, 1, 0x008000);
    for (x = 10 + az; x < 10; ++x)
      ctx.lamp.setPixelColorXY(x, 2, 0x800000);
    for (x = 10; x < 10 + az; ++x)
      ctx.lamp.setPixelColorXY(x, 2, 0x008000);
  }

  static constexpr bool hasCustomRamp = true;
};

} // namespace modes::custom::nudz
