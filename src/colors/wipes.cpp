#include "wipes.h"

#include <cstdint>

#include "../utils/constants.h"
#include "../utils/coordinates.h"
#include "../utils/utils.h"

namespace animations {

bool dot_wipe_down(const Color& color, const uint32_t duration,
                   const uint8_t fadeOut, const bool restart, LedStrip& strip,
                   const float cutOff) {
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart) {
    targetIndex = 0;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = ceil(LED_COUNT * cutOff);

  // convert duration in delay for each segment
  const unsigned long delay =
      max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT) + 2;

  if (targetIndex < LED_COUNT) {
    strip.fadeToBlackBy(fadeOut);
    // increment
    for (uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0;
         increment--) {
      strip.setPixelColor(
          targetIndex,
          color.get_color(targetIndex,
                          LED_COUNT));  //  Set pixel's color (in RAM)
      targetIndex += 1;
      if (targetIndex >= endIndex) break;
    }
  }

  return targetIndex >= endIndex;
}

bool dot_wipe_up(const Color& color, const uint32_t duration,
                 const uint8_t fadeOut, const bool restart, LedStrip& strip,
                 const float cutOff) {
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart) {
    targetIndex = LED_COUNT - 1;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = floor((1.0 - cutOff) * LED_COUNT) - 2;

  // convert duration in delay for each segment
  const unsigned long delay =
      max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT);

  if (targetIndex >= 0) {
    strip.fadeToBlackBy(fadeOut);
    // increment
    for (uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0;
         increment--) {
      strip.setPixelColor(
          targetIndex,
          color.get_color(targetIndex,
                          LED_COUNT));  //  Set pixel's color (in RAM)
      targetIndex -= 1;
      if (targetIndex == UINT16_MAX or targetIndex < endIndex) return true;
    }
  }

  return targetIndex == UINT16_MAX or targetIndex < endIndex;
}

bool color_wipe_down(const Color& color, const uint32_t duration,
                     const bool restart, LedStrip& strip, const float cutOff) {
  return dot_wipe_down(color, duration, 0, restart, strip);
}

bool color_wipe_up(const Color& color, const uint32_t duration,
                   const bool restart, LedStrip& strip, const float cutOff) {
  return dot_wipe_up(color, duration, 0, restart, strip);
}

bool color_vertical_wipe_right(const Color& color, const uint32_t duration,
                               const bool restart, LedStrip& strip) {
  static uint16_t currentX = 0;
  if (restart) {
    currentX = 0;
  }

  // convert duration in delay for each segment
  const unsigned long delay =
      max(LOOP_UPDATE_PERIOD, duration / stripXCoordinates);
  if (duration / stripXCoordinates <= LOOP_UPDATE_PERIOD) {
    for (uint16_t increment = stripXCoordinates / ceil(duration / delay);
         increment > 0; increment--) {
      for (uint16_t y = 0; y <= stripYCoordinates; ++y) {
        const auto pixelIndex = to_strip(currentX, y);
        strip.setPixelColor(pixelIndex, color.get_color(pixelIndex, LED_COUNT));
      }

      ++currentX;
      if (currentX > stripXCoordinates) {
        return true;
      }
    }
  } else  // delay is too long to increment cleanly, use gradients
  {
    static uint16_t lastSubstep = 0;
    static auto buffer1 = strip.get_buffer_ptr(0);
    const uint16_t maxSubstep =
        LOOP_UPDATE_PERIOD / (stripXCoordinates / duration * 1000.0);

    if (lastSubstep == 0) {
      strip.buffer_current_colors(0);
    }

    const float level = lastSubstep / (float)maxSubstep;

    for (uint16_t y = 0; y <= stripYCoordinates; ++y) {
      const auto pixelIndex = to_strip(currentX, y);
      strip.setPixelColor(
          pixelIndex,
          utils::get_gradient(buffer1[pixelIndex],
                              color.get_color(pixelIndex, LED_COUNT), level));
    }

    lastSubstep++;
    if (lastSubstep > maxSubstep) {
      lastSubstep = 0;
      ++currentX;
      if (currentX > stripXCoordinates) {
        return true;
      }
    }
  }
  return false;
}

};  // namespace animations