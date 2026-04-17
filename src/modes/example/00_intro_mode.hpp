/*! \file 00_intro_mode.hpp
    \brief Example implementation of the simplest possible mode.
*/

namespace lampda::modes::examples {

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

} // namespace lampda::modes::examples
