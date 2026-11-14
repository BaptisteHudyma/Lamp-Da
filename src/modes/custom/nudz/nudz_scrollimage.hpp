#include "src/modes/include/imu/utils.hpp"

namespace lampda::modes::custom::nudz {

enum RampModes : uint8_t
{
  Detached = 0,   ///< user ramp is not attached (thus, no effect)
  ImageSpeed = 1, ///< attach user ramp to image speed
  FrameSpeed = 2, ///< attach user ramp to animation frame speed
};

inline constexpr RampModes operator|(RampModes lhs, RampModes rhs)
{
  using T = std::underlying_type_t<RampModes>;
  return static_cast<RampModes>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline constexpr RampModes operator&(RampModes lhs, RampModes rhs)
{
  using T = std::underlying_type_t<RampModes>;
  return static_cast<RampModes>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

/**
 * \brief Display one or several overlaid scrolling images.
 * User ramp changes scroll speed and direction.
 *
 * Templated with a list of images. When several images are used, they are all
 * drawn in turn, so the 1st has better be opaque and the next ones have better
 * be masked with a transparent background (sprites).
 *
 * Images may be "animated" (series of images), an animation is considered a
 * singe image here.
 */
template<typename... ImageTypes> struct NudzScrollImageMode : public BasicMode
{
  /// anim frame sync mode
  enum FrameSyncMode
  {
    X,    ///< sync with 1st image x scrolling
    Y,    ///< sync with 1st image y scrolling
    Time, ///< sync with time, using speed (same as scroll speed)
  };

  typedef std::tuple_size<std::tuple<ImageTypes...>> TI;

  template<typename T> static constexpr std::array<T, TI::value> fill_array(const T& val)
  {
    std::array<T, TI::value> arr {};
    arr.fill(val);
    return arr;
  }

  template<typename T> using A = std::array<T, TI::value>;

  struct StateTy
  {
    A<float> minSpeed = fill_array(-0.5f);                       ///< minimum allowed speed
    A<float> maxSpeed = fill_array(0.5f);                        ///< maximum allowed speed
    A<float> minFrameSpeed = fill_array(-0.5f);                  ///< minimum allowed freme speed
    A<float> maxFrameSpeed = fill_array(0.5f);                   ///< maximum allowed frame speed
    A<RampModes> rampMode = fill_array(ImageSpeed | FrameSpeed); ///< ramp action mode
    A<bool> randomScroll = fill_array(false);                    ///< random variation of the scroll
    A<uint32_t> xdecal = fill_array(uint32_t(0));                ///< start x coordinates
    A<uint32_t> ydecal = fill_array(uint32_t(0));                ///< start y coordinates
    A<uint32_t> last_tick = fill_array(uint32_t(0));             ///< last called time in microseconds
    A<int8_t> xdirection = fill_array(int8_t(1));                ///< x scroll direction
    A<int8_t> ydirection = fill_array(int8_t(0));                ///< y scroll direction
    A<bool> framesMirror = fill_array(false);                    ///< frames are mirrored at end of animation
    A<NudzScrollImageMode::FrameSyncMode> syncFrame = fill_array(NudzScrollImageMode::Y); ///< frame sync on 1st image
    A<uint32_t> frame = fill_array(uint32_t(0));                                          ///< current animation frame
    A<uint32_t> last_frame_tick = fill_array(uint32_t(0)); ///< last time of frame change in microseconds
  };

  static void on_enter_mode(auto& ctx)
  {
    // reset stateful events
    uint32_t n = std::tuple_size<std::tuple<ImageTypes...>>::value;
    for (uint32_t i = 0; i < n; ++i)
    {
      ctx.state.xdecal[i] = 0;
      ctx.state.ydecal[i] = 0;
      ctx.state.last_tick[i] = 0;
    }

    /// prevent the ramp from looping around
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    /// call loop_image for each image in the template
    std::tuple<ImageTypes...> images;
    uint32_t itstate = 0;
    std::apply(
            [&](auto&&... arg) {
              ((loop_image(ctx, arg, itstate++)), ...);
            },
            images);
  }

  template<typename ImageType> static void loop_image(auto& ctx, const ImageType& image, const uint32_t& istate)
  {
    float spdRange = ctx.state.maxSpeed[istate] - ctx.state.minSpeed[istate];
    float speed =
            ctx.state.minSpeed[istate] +
            ((ctx.state.rampMode[istate] & ImageSpeed) ? (float(ctx.get_active_custom_ramp()) / 255) : 0) * spdRange;
    // ease stop at 0
    if (speed < -spdRange * 0.1)
      speed += spdRange * 0.1;
    else if (speed > spdRange * 0.1)
      speed -= spdRange * 0.1;
    else
      speed = 0.f;

    float frameSpdRange = ctx.state.maxFrameSpeed[istate] - ctx.state.minFrameSpeed[istate];
    float frameSpeed = ctx.state.minFrameSpeed[istate] +
                       ((ctx.state.rampMode[istate] & FrameSpeed) ? (float(ctx.get_active_custom_ramp()) / 255) : 0) *
                               frameSpdRange;
    // ease stop at 0
    if (frameSpeed < -frameSpdRange * 0.1)
      frameSpeed += frameSpdRange * 0.1;
    else if (FrameSpeed > frameSpdRange * 0.1)
      frameSpeed -= frameSpdRange * 0.1;
    else
      frameSpeed = 0.f;

    uint32_t frame = 0;

    uint16_t imWidth = ImageType::width[frame];
    uint16_t imHeight = ImageType::height[frame];

    // decal needs a state to keep continuous while speed is changed
    int32_t xdecal = 0;
    int32_t ydecal = 0;

    if (!ctx.state.randomScroll[istate])
    {
      xdecal = ctx.state.xdecal[istate] +
               int32_t((ctx.lamp.tick - ctx.state.last_tick[istate]) * speed * ctx.state.xdirection[istate]);
      ydecal = ctx.state.ydecal[istate] +
               int32_t((ctx.lamp.tick - ctx.state.last_tick[istate]) * speed * ctx.state.ydirection[istate]);

      switch (ctx.state.syncFrame[istate])
      {
        // force uint32_t - overflow is OK here.
        case NudzScrollImageMode::X:
          frame = uint32_t(istate == 0 ? xdecal : ctx.state.xdecal[0]);
          break;
        case NudzScrollImageMode::Y:
          frame = uint32_t(istate == 0 ? ydecal : ctx.state.ydecal[0]);
          break;
        default:
          frame = uint32_t(ctx.state.frame[istate] +
                           int32_t((ctx.lamp.tick - ctx.state.last_frame_tick[istate]) * frameSpeed));
      }
      if (ctx.state.frame[istate] != frame)
      {
        ctx.state.frame[istate] = frame;
        ctx.state.last_frame_tick[istate] = ctx.lamp.tick;
      }
      if (!ctx.state.framesMirror[istate] || ImageType::frames <= 2)
      {
        frame = frame % ImageType::frames;
      }
      else
      {
        frame = frame % (ImageType::frames * 2 - 2);
        if (frame >= ImageType::frames)
          frame = ImageType::frames * 2 - frame - 2;
      }

      imWidth = ImageType::width[frame];
      imHeight = ImageType::height[frame];
    }
    else // random scroll
    {
      xdecal = int32_t(ctx.state.xdecal[istate]) +
               int32_t((ctx.lamp.tick - ctx.state.last_tick[istate]) * speed * ctx.state.xdirection[istate]);
      ydecal = int32_t(ctx.state.ydecal[istate]) +
               int32_t((ctx.lamp.tick - ctx.state.last_tick[istate]) * speed * ctx.state.ydirection[istate]);

      if (xdecal < 0)
      {
        xdecal = 0;
        ctx.state.xdirection[istate] *= -random8(2);
        ctx.state.ydirection[istate] = random8(3) - 1;
        if (ctx.state.xdirection[istate] == 0 && ctx.state.ydirection[istate] == 0)
          ctx.state.ydirection[istate] = random8(2) * 2 - 1;
      }
      if (ydecal < 0)
      {
        ydecal = 0;
        ctx.state.ydirection[istate] *= -random8(2);
        ctx.state.xdirection[istate] = random8(3) - 1;
        if (ctx.state.xdirection[istate] == 0 && ctx.state.ydirection[istate] == 0)
          ctx.state.xdirection[istate] = random8(2) * 2 - 1;
      }

      switch (ctx.state.syncFrame[istate])
      {
        case NudzScrollImageMode::X:
          frame = istate == 0 ? xdecal : ctx.state.xdecal[0];
          break;
        case NudzScrollImageMode::Y:
          frame = istate == 0 ? ydecal : ctx.state.ydecal[0];
          break;
        default:
          frame = ctx.state.frame[istate] + int32_t((ctx.lamp.tick - ctx.state.last_frame_tick[istate]) * frameSpeed);
      }
      if (ctx.state.frame[istate] != frame)
      {
        ctx.state.frame[istate] = frame;
        ctx.state.last_frame_tick[istate] = ctx.lamp.tick;
      }
      if (!ctx.state.framesMirror[istate] || ImageType::frames <= 2)
      {
        frame = frame % ImageType::frames;
      }
      else
      {
        frame = frame % (ImageType::frames * 2 - 2);
        if (frame >= ImageType::frames)
          frame = ImageType::frames * 2 - frame - 2;
      }
      // frame = std::min(4, ImageType::frames - 1);  // DEBUG a fixed frame

      imWidth = ImageType::width[frame];
      imHeight = ImageType::height[frame];

      if (xdecal > 0 && xdecal + ctx.lamp.maxWidth >= imWidth)
      {
        xdecal = imWidth - ctx.lamp.maxWidth - 1;
        ctx.state.xdirection[istate] *= -random8(2);
        ctx.state.ydirection[istate] = random8(3) - 1;
        if (ctx.state.xdirection[istate] == 0 && ctx.state.ydirection[istate] == 0)
          ctx.state.ydirection[istate] = random8(2) * 2 - 1;
      }
      if (ydecal > 0 && ydecal + ctx.lamp.maxHeight >= imHeight)
      {
        ydecal = imHeight - ctx.lamp.maxHeight - 1;
        ctx.state.ydirection[istate] *= -random8(2);
        ctx.state.xdirection[istate] = random8(3) - 1;
        if (ctx.state.xdirection[istate] == 0 && ctx.state.ydirection[istate] == 0)
          ctx.state.xdirection[istate] = random8(2) * 2 - 1;
      }
    }

    if (xdecal != ctx.state.xdecal[istate] || ydecal != ctx.state.ydecal[istate])
    {
      ctx.state.xdecal[istate] = xdecal;
      ctx.state.ydecal[istate] = ydecal;
      ctx.state.last_tick[istate] = ctx.lamp.tick;
    }

    uint32_t w = min<uint32_t>(ctx.lamp.maxWidth + 1, imWidth);
    uint32_t h = min<uint32_t>(ctx.lamp.maxHeight, imHeight);
    uint32_t lx, ly;
    if (ImageType::colormapSize[frame] == 0)
    {
      uint32_t frameOffset = 0;
      for (auto i = 0; i < frame; ++i)
        frameOffset += ImageType::rgbFrameLength[i];

      for (uint32_t y = 0; y < h; ++y)
      {
        ly = (y + ImageType::y_offset[frame]) % ctx.lamp.maxHeight;
        for (uint32_t x = 0; x < w; ++x)
        {
          lx = (x + ImageType::x_offset[frame]) % (ctx.lamp.maxWidth + 1);
          uint32_t color =
                  ImageType::rgbData[((y + ydecal) % imHeight) * imWidth + (x + xdecal) % imWidth + frameOffset];
          if (ImageType::hasAlpha)
          {
            float opacity = float(color & 0xff) / 255;
            uint32_t prev_color = ctx.lamp.getPixelColor(ctx.lamp.fromXYtoStripIndex(lx, ly));
            color = (uint32_t((color >> 24) * opacity + (prev_color >> 16) * (1.f - opacity)) << 16) |
                    (uint32_t(((color & 0xff0000) >> 16) * opacity + ((prev_color & 0xff00) >> 8) * (1.f - opacity))
                     << 8) |
                    uint32_t(((color & 0xff00) >> 8) * opacity + (prev_color & 0xff) * (1.f - opacity));
          }
          else
            color = color >> 8;
          ctx.lamp.setPixelColorXY(lx, ly, color);
        }
      }
    }
    else
    {
      // indexed colormap
      uint32_t frameOffset = 0;
      for (auto i = 0; i < frame; ++i)
        frameOffset += ImageType::indexFrameLength[i];

      uint8_t bmask = (1 << ImageType::bitsPerPixel[frame]) - 1;
      for (uint32_t y = 0; y < h; ++y)
      {
        ly = (y + ImageType::y_offset[frame]) % ctx.lamp.maxHeight;
        uint32_t yoffset = ((y + ydecal) % imHeight) * imWidth;
        for (uint32_t x = 0; x < w; ++x)
        {
          lx = (x + ImageType::x_offset[frame]) % (ctx.lamp.maxWidth + 1);
          uint32_t offset = yoffset + (x + xdecal) % imWidth;
          uint32_t byteOffset = offset * ImageType::bitsPerPixel[frame] / 8;
          uint32_t bitOffset = (offset * ImageType::bitsPerPixel[frame]) % 8;
          uint8_t index = ImageType::indexData[byteOffset + frameOffset];
          if (bitOffset + ImageType::bitsPerPixel[frame] > 8)
          {
            uint16_t sindex = (index << 8) | ImageType::indexData[byteOffset + 1 + frameOffset];
            index = (sindex >> (16 - bitOffset - ImageType::bitsPerPixel[frame])) & bmask;
          }
          else
            index = (index >> (8 - bitOffset - ImageType::bitsPerPixel[frame])) & bmask;

          uint32_t cmap_off = 0;
          for (auto i = 0; i < frame; ++i)
            cmap_off += ImageType::colormapSize[i];
          uint32_t color = ImageType::colormap[index + cmap_off];
          if (ImageType::hasAlpha)
          {
            float opacity = float(color & 0xff) / 255;
            uint32_t prev_color = ctx.lamp.getPixelColor(ctx.lamp.fromXYtoStripIndex(lx, ly));
            color = (uint32_t((color >> 24) * opacity + (prev_color >> 16) * (1.f - opacity)) << 16) |
                    (uint32_t(((color & 0xff0000) >> 16) * opacity + ((prev_color & 0xff00) >> 8) * (1.f - opacity))
                     << 8) |
                    uint32_t(((color & 0xff00) >> 8) * opacity + (prev_color & 0xff) * (1.f - opacity));
          }
          ctx.lamp.setPixelColorXY(lx, ly, color);
        }
      }
    }
  }

  /// Hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;
};

#include "src/generated/heineken.hpp"

typedef NudzScrollImageMode<HeinekenImageTy> NudzHeinekenMode;

#include "src/generated/huit_six.hpp"

typedef NudzScrollImageMode<Huit_sixImageTy> NudzHuitSixMode;

#include "src/generated/violonsaouls.hpp"

/**
 * \brief Display an image "ViolonSaouls" scrolling around.
 * User ramp changes scroll speed and direction
 */
struct NudzViolonsaoulsMode : public NudzScrollImageMode<ViolonsaoulsImageTy>
{
  struct StateTy
  {
    A<float> minSpeed = {0.f};            ///< minimum allowed speed
    A<float> maxSpeed = {0.5f};           ///< maximum allowed speed
    A<float> minFrameSpeed = {0.f};       ///< minimum allowed freme speed
    A<float> maxFrameSpeed = {0.1f};      ///< maximum allowed frame speed
    A<RampModes> rampMode = {ImageSpeed}; ///< ramp action mode
    A<bool> randomScroll = {false};       ///< random variation of the scroll
    A<uint32_t> xdecal = {0};             ///< start x coordinates
    A<uint32_t> ydecal = {0};             ///< start y coordinates
    A<uint32_t> last_tick = {0};          ///< last called time in microseconds
    A<int8_t> xdirection = {1};           ///< x scroll direction
    A<int8_t> ydirection = {0};           ///< y scroll direction
    A<bool> framesMirror = {false};       ///< frames are mirrored at end of animation
    A<NudzScrollImageMode::FrameSyncMode> syncFrame = {NudzScrollImageMode::Y}; ///< frame sync on 1st image
    A<uint32_t> frame = {0};                                                    ///< current animation frame
    A<uint32_t> last_frame_tick = {0};
  };
};

/**
 * \brief Display a glass of beer, reactive to gravity.
 * The user ramp changes the beer level
 */
struct NudzBeerGlassMode : public BasicMode
{
  struct BubbleTy
  {
    BubbleTy() : x(-1.f), y(0.f), speed(0.f), color(0x706050), color_fade(0.9) {}
    int x;            ///< x coordinate
    float y;          ///< y coordinate
    float speed;      ///< speed in px/second
    uint32_t color;   ///< color of this buble
    float color_fade; ///< fade rate
  };

  struct StateTy
  {
    static constexpr uint32_t nbubbles = 30; ///< number of bubbles

    float level;         ///< average level of beer
    float ampl;          ///< accel amplitude factor
    float accmax;        ///< clamp accel to avoid too large changes
    float fall_ampl;     ///< height a wave will go down at each step
    float wave_ampl;     ///< wave speed
    float decay;         ///< speed decay factor
    float bounce_ratio;  ///< if ratio > 0.5 a wave dropping can increase. Its neighbours height over its new height,
                         ///< thus produce a new wave
    uint32_t beer_color; ///< color of the beer :)
    uint32_t foam_color; ///< color of the foam of the beer
    uint32_t background_color;                  ///< color background
    std::array<float, LampTy::maxWidth> levels; ///< level of every matrix columns
    std::array<float, LampTy::maxWidth> speeds; ///< spped of every matric columns
    std::array<BubbleTy, nbubbles> bubbles;     ///< store the bubbles

    imu::ImuEventTy<> imuEvent; ///< Handle imu events
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.imuEvent.reset(ctx);

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
    ctx.state.levels.fill(ctx.state.level);
    ctx.state.speeds.fill(0.f);
    ctx.state.bubbles.fill(BubbleTy());

    /// prevent the ramp from looping around
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
  }

  static void loop(auto& ctx)
  {
    ctx.state.imuEvent.update(ctx);

    updateLevels(ctx);

    // display
    displayLevels(ctx);

    // bubbles
    makeBubbles(ctx);

    // draw accel
    if (ctx.get_active_custom_ramp() >= 240)
      drawAccel(ctx);
  }

  /// update the level of the liquid using IMU events
  static void updateLevels(auto& ctx)
  {
    uint32_t nx = ctx.lamp.maxWidth;
    uint32_t ny = ctx.lamp.maxHeight;

    const auto& reading = ctx.state.imuEvent.lastReading;
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

    utils::vec2d acc(accel.x, accel.y);
    // threshold because there is a drift in the accel
    if (acc.x * acc.x + acc.y * acc.y < 4.f)
      acc = utils::vec2d(0.f, 0.f);
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

    std::array<float, LampTy::maxWidth> ospeeds = speeds;

    // accelerate column by column
    // the "column" model is very simple and easy,
    // but cannot account for real orientation changes
    // since the Z (column) axis is always considered to be vertical
    for (uint32_t x = 0; x < nx; ++x)
    {
      // angular coords of the pixel
      float angle = float(x) / nx * M_PI * 2;
      // normal and tangent to the lamp
      utils::vec2d norm(cos_t(angle), sin_t(angle));
      utils::vec2d tan(-norm.y, norm.x);
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
        diff = min<float>(ld, ctx.state.fall_ampl);
      else if (ld < 0)
        diff = max<float>(ld, -ctx.state.fall_ampl);
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

  /// Display the liquid level on the strip
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

  /// Invoque new bubbles !
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

  /// Draw the acceleration vector from the IMU
  static void drawAccel(auto& ctx)
  {
    const auto& reading = ctx.state.imuEvent.lastReading;
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

  /// hint manager that we have a custom ramp
  static constexpr bool hasCustomRamp = true;
};

} // namespace lampda::modes::custom::nudz
