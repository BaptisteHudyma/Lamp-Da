#include "wipes.h"

#include "colors.h"
#include "constants.h"
#include "utils.h"

namespace animations
{

bool dotWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;
  static unsigned long previousMillis = 0;

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    previousMillis = 0;
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= ceil(LED_COUNT * cutOff))
    return true;

  // convert duration in delay for each segment
  const uint16_t delay = duration / (float)LED_COUNT;

  const unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex < LED_COUNT) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex, color.get_color(targetIndex, LED_COUNT));  //  Set pixel's color (in RAM)
    targetIndex += 1;
    strip.show();  //  Update strip to match
  }

  return targetIndex >= ceil(LED_COUNT * cutOff);
}

bool dotWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;
  static unsigned long previousMillis = 0;

  // reset condition
  if (restart)
  {
    targetIndex = LED_COUNT - 1;
    previousMillis = 0;
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex == UINT16_MAX or targetIndex < floor((1.0 - cutOff) * LED_COUNT))
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)LED_COUNT;

  const unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex >= 0) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex, color.get_color(targetIndex, LED_COUNT));  //  Set pixel's color (in RAM)
    targetIndex -= 1;
    strip.show();  //  Update strip to match
  }

  return targetIndex == UINT16_MAX or targetIndex < floor((1.0 - cutOff) * LED_COUNT);
}

bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static uint16_t targetIndex = UINT16_MAX;
  static unsigned long previousMillis = 0;
  static uint32_t ledStates[LED_COUNT];
  static uint8_t fadeLevel = 0;

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    previousMillis = 0;
    fadeLevel = 0;

    // save initial state
    for(uint16_t i = 0; i < LED_COUNT; ++i)
      ledStates[i] = strip.getPixelColor(i);
    
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= ceil(LED_COUNT * cutOff))
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)LED_COUNT;
  const uint32_t c = color.get_color(targetIndex, LED_COUNT);

  const unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;
    fadeLevel = 0;

    strip.setPixelColor(targetIndex++, c);
    strip.show();  //  Update strip to match
  }
  else
  {
    const float coeff = (currentMillis - previousMillis) / (float)delay;
    const uint8_t newFadelevel = coeff * 255;
    if (newFadelevel != fadeLevel)
    {
      fadeLevel = newFadelevel;
      // update the value of the last segement with a gradient
      strip.setPixelColor(targetIndex, utils::get_gradient(ledStates[targetIndex], c, coeff));
      strip.show();  //  Update strip to match
    }
  }

  return targetIndex >= ceil(LED_COUNT * cutOff);
}

bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static uint32_t ledStates[LED_COUNT];
  static uint16_t targetIndex = UINT16_MAX;
  static unsigned long previousMillis = 0;
  static uint8_t fadeLevel = 0;

  // reset condition
  if (restart)
  {
    targetIndex = LED_COUNT - 1;
    previousMillis = 0;
    fadeLevel = 0;

    // save initial state
    for(uint16_t i = 0; i < LED_COUNT; ++i)
      ledStates[i] = strip.getPixelColor(i);
    
    return false;
  }

  const unsigned long currentMillis = millis();
  // finished if the target index is over the led limit
  if (targetIndex == UINT16_MAX or targetIndex < floor((1.0 - cutOff) * LED_COUNT))
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)LED_COUNT;
  const uint32_t c = color.get_color(targetIndex, LED_COUNT);

  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;
    fadeLevel = 0;

    strip.setPixelColor(targetIndex--, c);
    strip.show();  //  Update strip to match
  }
  else
  {
    const float coeff = (currentMillis - previousMillis) / (float)delay;
    const uint8_t newFadelevel = coeff * 255;
    if (newFadelevel != fadeLevel)
    {
      fadeLevel = newFadelevel;
      // update the value of the last segement with a gradient
      strip.setPixelColor(targetIndex, utils::get_gradient(ledStates[targetIndex], c, coeff));
      strip.show();  //  Update strip to match
    }
  }

  return targetIndex == UINT16_MAX or targetIndex < floor((1.0 - cutOff) * LED_COUNT);
}

};