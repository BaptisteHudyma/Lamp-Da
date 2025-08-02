#ifndef MODES_INCLUDE_ANIMS_RAMP_UPDATE
#define MODES_INCLUDE_ANIMS_RAMP_UPDATE

#include "src/modes/include/colors/utils.hpp"

namespace modes::anims {

/** \brief Display a ramp animation while freezing the active mode
 *
 * An usage example in a custom mode would be:
 *
 * @code{.cpp}
 *
 * static void custom_ramp_update(auto& ctx, uint8_t rampValue) {
 *   modes::colors::rampRainbow(ctx, rampValue);
 * }
 *
 * static constexpr bool hasCustomRamp = true;
 * @endcode
 *
 * You can configure \p freezeSize to adjust how many frames are skipped.
 *
 */
template<uint8_t freezeSize = 5, bool forwardRamp = true> void rampRainbow(auto& ctx, uint8_t rampValue)
{
  uint16_t overlaySize = (ctx.lamp.ledCount * rampValue) / 256;
  for (uint16_t i = 0; i < overlaySize; ++i)
  {
    uint16_t colorAngle = (i * 360) / overlaySize + (forwardRamp ? rampValue * 4 : 0);
    auto color = modes::colors::fromAngleHue(colorAngle);
    ctx.lamp.setPixelColor(i, color);
  }

  for (uint16_t i = 0; i < 5; ++i)
  {
    if (i == 0 && overlaySize < ctx.lamp.ledCount)
    {
      ctx.lamp.setPixelColor(overlaySize + i, modes::colors::fromRGB(255, 255, 255));
      continue;
    }

    if (overlaySize + i < ctx.lamp.ledCount)
    {
      ctx.lamp.setPixelColor(overlaySize + i, modes::colors::fromRGB(0, 0, 0));
    }
  }

  ctx.skipNextFrames(freezeSize);
}

/** \brief Display a ramp animation on the first LEDs with the active mode
 *
 * An usage example in a custom mode would be:
 *
 * @code{.cpp}
 *
 * static void custom_ramp_update(auto& ctx, uint8_t rampValue) {
 *   modes::colors::rampColorRing(ctx, rampValue, modes::colors::PaletteBlackBodyColors);
 * }
 *
 * static constexpr bool hasCustomRamp = true;
 * @endcode
 *
 * You can configure \p nbBlackLines to adjust how many lines are skipped.
 *
 */
template<uint8_t freezeSize = 5, uint8_t nbBlackLines = 2>
void rampColorRing(auto& ctx, uint8_t rampValue, auto palette)
{
  ctx.skipFirstLedsForFrames(0);

  for (uint8_t i = 0; i < ctx.lamp.maxWidth * nbBlackLines - 1; ++i)
  {
    ctx.lamp.setPixelColor(i, 0);
  }

  for (uint16_t i = 0; i < ctx.lamp.maxWidth - 1; ++i)
  {
    uint8_t rampScale = (i * 255) / ctx.lamp.maxWidth;
    if (rampScale < rampValue)
    {
      const auto color = modes::colors::from_palette(rampScale, palette);
      ctx.lamp.setPixelColor(i, color);
    }
    else
    {
      ctx.lamp.setPixelColor(i, modes::colors::Black);
    }
  }

  ctx.skipFirstLedsForFrames(ctx.lamp.maxWidth * nbBlackLines, freezeSize);
}

/// \private (dispatch ramp animation)
void inline LMBD_INLINE _rampAnimDispatch(uint32_t index, auto& ctx, uint8_t rampValue)
{
  if (index == 0)
  {
    rampColorRing(ctx, rampValue, modes::colors::PaletteBlackBodyColors);
  }
  else if (index == 1)
  {
    rampRainbow(ctx, rampValue);
  }
  else
  {
    return; // let's add other rampColorRing variants :)
  }
}

} // namespace modes::anims

#endif
