
namespace modes::examples {

/**
 * \brief Extremly basic mode, shown has an exemple
 */
struct IntroMode : public BasicMode
{
  static void loop(auto& ctx)
  {
    ctx.lamp.fill(colors::Chartreuse);
    ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.getMaxBrightness());
  }
};

} // namespace modes::examples
