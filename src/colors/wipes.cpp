#include "wipes.h"

#include "../utils/constants.h"
namespace animations
{

bool dotWipeDown(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = ceil(LED_COUNT * cutOff);

  // convert duration in delay for each segment
  const unsigned long delay = max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT) + 2;

  if (targetIndex < LED_COUNT) {

    strip.clear();
    // increment
    for(uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0; increment--)
    {
      strip.setPixelColor(targetIndex, color.get_color(targetIndex, LED_COUNT));  //  Set pixel's color (in RAM)
      targetIndex += 1;
      if (targetIndex >= endIndex)
        break;
    }
  }

  return targetIndex >= endIndex;
}

bool dotWipeUp(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart)
  {
    targetIndex = LED_COUNT - 1;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = floor((1.0 - cutOff) * LED_COUNT) - 2;

  // convert duration in delay for each segment
  const unsigned long delay = max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT);

  if (targetIndex >= 0) {

    strip.clear();
    // increment 
    for(uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0; increment--)
    {
      strip.setPixelColor(targetIndex, color.get_color(targetIndex, LED_COUNT));  //  Set pixel's color (in RAM)
      targetIndex -= 1;
      if (targetIndex == UINT16_MAX or targetIndex < endIndex)
        return true;
    }
  }

  return targetIndex == UINT16_MAX or targetIndex < endIndex;
}

bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = ceil(LED_COUNT * cutOff) + 2;

  // convert duration in delay for each segment
  const unsigned long delay = max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT);
  const uint32_t c = color.get_color(targetIndex, LED_COUNT);

  // increment 
  for(uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0; increment--)
  {
    strip.setPixelColor(targetIndex++, c);
    if (targetIndex > endIndex)
      break;
  }

  return targetIndex > endIndex;
}

bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;

  // reset condition
  if (restart)
  {
    targetIndex = LED_COUNT - 1;
    return false;
  }

  // finished if the target index is over the led limit
  const uint16_t endIndex = floor((1.0 - cutOff) * LED_COUNT) - 2;

  // convert duration in delay for each segment
  const unsigned long delay = max(LOOP_UPDATE_PERIOD, duration / (float)LED_COUNT);
  const uint32_t c = color.get_color(targetIndex, LED_COUNT);

  // increment 
  for(uint32_t increment = LED_COUNT / ceil(duration / delay); increment > 0; increment--)
  {
    strip.setPixelColor(targetIndex--, c);
    if(targetIndex == UINT16_MAX or targetIndex < endIndex)
      break;
  }

  return targetIndex == UINT16_MAX or targetIndex < endIndex;
}

};