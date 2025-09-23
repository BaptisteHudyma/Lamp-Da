#ifndef MODES_DRAW_GRID_RULE_HPP
#define MODES_DRAW_GRID_RULE_HPP

#include <array>

namespace modes::draw::grid {

using LampTy = hardware::LampTy;

struct LineRuleConfig
{
  static constexpr uint8_t dstBufIdx = 0;         /// Main buf. used for the grid
  static constexpr uint8_t srcBufIdx = 1;         /// Sec. buf. used for smooth grid
  static constexpr uint8_t scrollAmount = 1;      /// Scroll each update
  static constexpr uint8_t renderBlurAmount = 32; /// Blur before display
  static constexpr bool scrollSkewed = false;     /// Scroll skewed to the left
};

/* \brief Implement a line-based grid pattern, update line per line.
 *
 * Typical usage is:
 *
 * @code{.cpp}
 *
 *   static void loop(auto& ctx)
 *   {
 *     auto cb = [&](const auto& before, auto& after) {
 *      // read array "before" for previous line, write updated line to "after" array
 *     };
 *
 *     ctx.state.grid.update(ctx.lamp, cb);
 *     ctx.state.grid.display(ctx.lamp);
 *   }
 *
 * @endcode
 *
 * See examples to learn more about the behavior of this.
 */
template<typename ConfigTy = LineRuleConfig> struct LineRule
{
  static constexpr uint8_t dstBufIdx = ConfigTy::dstBufIdx; /// \private
  static constexpr uint8_t srcBufIdx = ConfigTy::srcBufIdx; /// \private

  static constexpr float fwidth = LampTy::maxWidthFloat; /// \private
  static constexpr uint16_t width = LampTy::maxWidth;    /// \private
  using LineTy = std::array<uint32_t, width>;            // Type of a line array

  /// Number of lines in grid
  static constexpr uint16_t nbLines = LampTy::maxHeight;

  /// To be called in the parent .reset() to set \p firstLine as first line
  void reset(LampTy& lamp, LineTy& firstLine)
  {
    lastLine = 0;
    currentLine = firstLine;

    lamp.template fillTempBuffer(0);
    auto& buffer = lamp.template getTempBuffer<dstBufIdx>();
    std::copy(currentLine.begin(), currentLine.end(), buffer.begin());
  }

  /// Reset grip with an empty (black) first line
  void LMBD_INLINE reset(LampTy& lamp)
  {
    LineTy first {};
    reset(lamp, first);
  }

  /// When called, pass \p before and \p after line to callback for processing
  void update(LampTy& lamp, auto& callback)
  {
    auto& buffer = lamp.template getTempBuffer<dstBufIdx>();

    uint16_t nextLine = (lastLine + 1) % (nbLines + 1);
    uint16_t nextStart = nextLine * fwidth;
    bool endReached = (nextStart + width) > buffer.size();

    // if scrollAmount is 0, then we prefer wrapping the grid
    if (endReached && ConfigTy::scrollAmount == 0)
    {
      nextStart = 0;
      lastLine = (1 + lastLine) % (nbLines + 1);
    }

    // if not, scroll grid by scrollAmount (skewed or not)
    else if (endReached)
    {
      scrollBy(lamp, ConfigTy::scrollAmount, ConfigTy::scrollSkewed);

      lastLine -= ConfigTy::scrollAmount;
      nextLine -= ConfigTy::scrollAmount;
      nextStart = nextLine * fwidth;
    }

    // call callback
    LineTy newLine {};
    callback(currentLine, newLine);
    currentLine = newLine;

    // save result to buffer & move to next line
    if (assertBool(nextStart + width <= buffer.size()))
      std::copy(newLine.begin(), newLine.end(), buffer.begin() + nextStart);

    lastLine = (1 + lastLine) % (nbLines + 1);
  }

  /// Display the line-rule grid onto screen (if \p reversed reverse it)
  void display(LampTy& lamp, bool reversed = false)
  {
    if (reversed)
    {
      lamp.template setColorsFromBufferReversed<dstBufIdx>(true);
    }
    else
    {
      lamp.template setColorsFromBuffer<dstBufIdx>();
    }
  }

  /// Same as \p .update() but uses two buffers, required for \p smoothDisplay()
  void LMBD_INLINE smoothUpdate(LampTy& lamp, auto& callback)
  {
    auto bufCopy = lamp.template getTempBuffer<dstBufIdx>();
    update(lamp, callback);
    lamp.template getTempBuffer<srcBufIdx>() = bufCopy;
  }

  /// Display grid buffers as an in-between frame, at \p phasis (from 0 to 1)
  void LMBD_INLINE smoothDisplay(LampTy& lamp, float phasis)
  {
    lamp.setColorsFromMixedBuffers<dstBufIdx, srcBufIdx>(phasis);
  }

  /// For a \p counter between 0 and \p maxCounter display a smooth frame
  void LMBD_INLINE smoothDisplay(LampTy& lamp, uint16_t counter, uint16_t maxCounter, uint16_t maxValue = 256)
  {
    uint16_t increment = maxValue / maxCounter;
    float phase = counter * increment + increment / 2.0;
    smoothDisplay(lamp, phase / maxValue);
  }

  /// Return a copy of line at \p lineAtIndex (may be empty if out of screen)
  LineTy LMBD_INLINE lineAtIndex(LampTy& lamp, uint16_t lineIndex)
  {
    const auto& buffer = lamp.template getTempBuffer<dstBufIdx>();
    LineTy newLine {};

    if (lineIndex < nbLines)
    {
      uint16_t lineStart = lineIndex * fwidth;
      if (assertBool(lineStart + width <= buffer.size()))
        std::copy(buffer.cbegin() + lineStart, buffer.cbegin() + lineStart + width, newLine.begin());
    }

    return newLine;
  }

  /// Scroll grid by \p amount upward (optionnaly skewed to the left)
  void scrollBy(LampTy& lamp, uint8_t amount, bool skewed)
  {
    auto& buffer = lamp.template getTempBuffer<dstBufIdx>();

    for (uint16_t I = 0; I < nbLines; ++I)
    {
      auto line = lineAtIndex(lamp, I + amount);
      uint16_t shiftStart = I * fwidth;
      uint16_t shiftTweak = I * fwidth + 0.5;

      // unholy coordinate system w/ LEDs 0.041151pt too long
      if (skewed && shiftStart == shiftTweak && (I > 2 && I != lamp.shiftResidue))
      {
        auto last = line[0];
        std::copy(line.begin() + 1, line.end(), line.begin());
        line[line.size() - 1] = last;
      }

      // (at that point I gave up finding why)
      if (skewed && I == 2)
        shiftStart -= 1;

      if (assertBool(shiftStart + line.size() <= buffer.size()))
        std::copy(line.begin() + ((I || !skewed) ? 0 : 1), line.end(), buffer.begin() + shiftStart);
    }
  }

  /* \brief Default loop function, update and display, smooth display if fast
   *
   * If \p hasCustomRamp is True, uses internally custom ramp to configure
   * update speed of the grid. If fast and high luminosity, try smoothing the
   * display, if slow or low luminosity, disable smoothing to avoid user seeing
   * a blurry "marching effect" between updates.
   *
   * If \p hasCustomRamp is False, use \p rampSubstitute as value, higher being
   * slower (from 0 to 256). Use \p baseSmooth to configure how fast the smooth
   * mode can go, value 0 being the fastest, and 10 disabling smoothing.
   *
   */
  template<bool hasCustomRamp, uint8_t rampSubstitute = 70, uint8_t baseSmooth = 4>
  void LMBD_INLINE loop(auto& ctx, auto& callback)
  {
    auto& lamp = ctx.lamp;
    uint8_t rampValue = (hasCustomRamp ? ctx.get_active_custom_ramp() : rampSubstitute);

    // by default, quickly run smoothly the grid scrolling
    uint16_t smoothFrame = baseSmooth + rampValue / 16;
    if (smoothFrame <= 10 && lamp.getBrightness() > 32)
    {
      uint32_t counter = lamp.raw_frame_count % smoothFrame;
      if (!counter)
        smoothUpdate(lamp, callback);

      smoothDisplay(lamp, counter, smoothFrame);
      if constexpr (ConfigTy::renderBlurAmount)
        lamp.blur(ConfigTy::renderBlurAmount);

      // coarser display for slow scrolling (smoothing too visible)
    }
    else
    {
      int now = lamp.now;
      int frameDuration = rampValue + lamp.frameDurationMs;

      if (abs(int(now - lastUpdate)) > frameDuration)
      {
        lastUpdate = now;
        update(lamp, callback);

        display(lamp);
        if constexpr (ConfigTy::renderBlurAmount)
          lamp.blur(ConfigTy::renderBlurAmount);
      }
    }
  }

  LineTy currentLine;
  uint16_t lastLine = 0;
  uint32_t lastUpdate = 0;
};

/** \brief Process 1-d cellular automata from \p before to \p after array
 *
 * In order:
 *
 *  - \p ruleNo is the wolframe rule number
 *  - \p straight if True, skew the automata, to get triangles to be "straight"
 *  - \p leftBound if True, do not wrap arrays on the left boundary
 *  - \p rightBound if TRue, do not wrap arrays on the right boundary
 *  - \p nbBits process only some (24 default) lower bits of the input arrays
 */
template<uint8_t ruleNo, bool straight = false, bool leftBound = false, bool rightBound = false, uint8_t nbBits = 24>
void wolframRule(const auto& before, auto& after)
{
  constexpr uint8_t pattern[8] = {0b1 & (ruleNo >> 7),
                                  0b1 & (ruleNo >> 6),
                                  0b1 & (ruleNo >> 5),
                                  0b1 & (ruleNo >> 4),
                                  0b1 & (ruleNo >> 3),
                                  0b1 & (ruleNo >> 2),
                                  0b1 & (ruleNo >> 1),
                                  0b1 & (ruleNo >> 0)};

  uint16_t start = leftBound ? 1 : 0;
  uint16_t end = rightBound ? after.size() - 1 : after.size();

  for (uint16_t I = start; I < end; ++I)
  {
    uint16_t J = (I + (straight ? 1 : 0)) % end;
    after[J] = 0;

    for (uint32_t P = 0; P < nbBits; ++P)
    {
      uint32_t mask = 1 << P;
      uint32_t l = before[(I + before.size() - 1) % before.size()] & mask;
      uint32_t m = before[I] & mask;
      uint32_t r = before[(I + 1) % before.size()] & mask;

      uint8_t lmr = 0;
      if (l)
        lmr |= 0b100;
      if (m)
        lmr |= 0b010;
      if (r)
        lmr |= 0b001;

      if (pattern[lmr])
      {
        after[J] |= mask;
      }
    }
  }
}

} // namespace modes::draw::grid

#endif
