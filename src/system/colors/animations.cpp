#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "animations.h"

#include <math.h>
#include <sys/types.h>

#include <cmath>
#include <cstdint>

#include "src/system/colors/palettes.h"
#include "src/system/colors/wipes.h"
#include "src/system/colors/text.h"
#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"
#include "src/system/ext/random8.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/coordinates.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/time.h"

namespace animations {

void fill(const Color& color, LedStrip& strip, const float cutOff)
{
  const float adaptedCutoff = lmpd_constrain<float>(cutOff, 0.0f, 1.0f);
  const uint16_t maxCutOff = lmpd_constrain<uint16_t>(adaptedCutoff * LED_COUNT, 1.0, LED_COUNT);
  for (uint16_t i = 0; i < LED_COUNT; ++i)
  {
    const uint32_t c = color.get_color(i, LED_COUNT);
    if (i >= maxCutOff)
    {
      // set a color gradient
      float intPart, fracPart;
      fracPart = modf(adaptedCutoff * LED_COUNT, &intPart);
      // adapt the color to have a nice gradient
      COLOR newC;
      newC.color = c;
      const uint16_t hue = utils::ColorSpace::HSV(newC).get_scaled_hue();
      strip.setPixelColor(i, LedStrip::ColorHSV(hue, 255, fracPart * 255));
      break;
    }

    strip.setPixelColor(i, c);
  }
}

bool dot_ping_pong(const Color& color,
                   const uint32_t duration,
                   const uint8_t fadeOut,
                   const bool restart,
                   LedStrip& strip,
                   const float cutOff)
{
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    dot_wipe_up(color, duration / 2, fadeOut, true, strip);
    dot_wipe_down(color, duration / 2, fadeOut, true, strip);
  }

  if (isPongMode)
  {
    return dot_wipe_up(color, duration / 2, fadeOut, false, strip, cutOff);
  }
  else
  {
    // set pong mode to true when first animation is finished
    isPongMode = dot_wipe_down(color, duration / 2, fadeOut, false, strip, cutOff);
  }

  // finished if the target index is over the led limit
  return false;
}

bool double_side_fill(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip)
{
  if (restart)
  {
    color_wipe_up(color, duration, true, strip, 0.51);
    color_wipe_down(color, duration, true, strip, 0.51);
  }

  return color_wipe_down(color, duration, false, strip, 0.51) or color_wipe_up(color, duration, false, strip, 0.51);
}

bool police(const uint32_t duration, const bool restart, LedStrip& strip)
{
  static unsigned long previousMillis = 0;
  static uint8_t state = 0;

  if (restart)
  {
    previousMillis = 0;
    state = 0;
  }

  // convert duration in delay
  const uint16_t partDelay = duration / 3;

  const uint32_t bluePartLenght = 3.0 / 5.0 * partDelay;
  const uint32_t clearPartLenght = 2.0 / 5.0 * bluePartLenght;

  const unsigned long currentMillis = time_ms();

  switch (state)
  {
    // first left blue flash
    case 0:
      {
        strip.clear();

        // blue lights
        strip.fill(LedStrip::Color(0, 0, 255), 0,
                   LED_COUNT / 2 + 1); // Set on the first part

        // set next state
        state++;
        break;
      }
    // first short clear
    case 1:
      {
        if (currentMillis - previousMillis >= bluePartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();

          // set next state
          state++;
        }
        break;
      }
    // second left blue flash
    case 2:
      {
        if (currentMillis - previousMillis >= bluePartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();

          // blue lights
          strip.fill(LedStrip::Color(0, 0, 255), 0,
                     LED_COUNT / 2 + 1); // Set on the first part

          // set next state
          state++;
        }
        break;
      }

    // first right blue flash
    case 3:
      {
        if (currentMillis - previousMillis >= clearPartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();
          strip.fill(LedStrip::Color(0, 0, 255), LED_COUNT / 2,
                     LED_COUNT); // Set on the second part

          // set next state
          state++;
        }
        break;
      }
    // first short pause
    case 4:
      {
        if (currentMillis - previousMillis >= bluePartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();

          // set next state
          state++;
        }
        break;
      }
    case 5:
      {
        if (currentMillis - previousMillis >= bluePartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();
          strip.fill(LedStrip::Color(0, 0, 255), LED_COUNT / 2,
                     LED_COUNT); // Set on the second part

          // set next state
          state++;
        }
        break;
      }
    case 6:
      {
        if (currentMillis - previousMillis >= bluePartLenght)
        {
          previousMillis = currentMillis;

          strip.clear();
          state = 0;
          return true;
        }
        break;
      }
  }

  // never ends
  return false;
}

// effect utility functions
uint8_t sin_gap(uint16_t in)
{
  if (in & 0x100)
    return 0;
  return sin8(in + 192); // correct phase shift of sine so that it starts and stops at 0
}

void show_text(const Color& color, const std::string& text, LedStrip& strip)
{
  static bool isOver = true;

  isOver = text::display_scrolling_text(color, text, 4, 1, 2000, isOver, true, 200, strip);
}

} // namespace animations

#endif
