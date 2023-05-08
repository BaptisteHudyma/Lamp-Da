#include "wipes.h"

#include "colors.h"
#include "constants.h"
#include "utils.h"

namespace animations
{

bool dotWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static uint16_t targetIndex = 0;
  static unsigned long previousMillis = 0;
  const uint16_t numberOfLedSegments = strip.numPixels();

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    previousMillis = 0;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const uint16_t delay = duration / (float)numberOfLedSegments;

  const unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex < numberOfLedSegments) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex, color.get_color(targetIndex, numberOfLedSegments));  //  Set pixel's color (in RAM)
    targetIndex += 1;
    strip.show();  //  Update strip to match
  }

  return targetIndex >= numberOfLedSegments;
}

bool dotWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  const uint16_t numberOfLedSegments = strip.numPixels();
  static uint16_t targetIndex = numberOfLedSegments - 1;
  static unsigned long previousMillis = 0;

  // reset condition
  if (restart)
  {
    targetIndex = numberOfLedSegments - 1;
    previousMillis = 0;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;

  const unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex >= 0) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex--, color.get_color(targetIndex, numberOfLedSegments));  //  Set pixel's color (in RAM)
    strip.show();  //  Update strip to match
  }

  return targetIndex >= numberOfLedSegments;
}

bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static uint16_t targetIndex = 0;
  static unsigned long previousMillis = 0;
  const uint16_t numberOfLedSegments = strip.numPixels();
  static uint32_t ledStates[LED_COUNT];
  static uint8_t fadeLevel = 0;

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    previousMillis = 0;
    fadeLevel = 0;

    // save initial state
    for(uint16_t i = 0; i < numberOfLedSegments; ++i)
      ledStates[i] = strip.getPixelColor(i);
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;
  const uint32_t c = color.get_color(targetIndex, numberOfLedSegments);

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

  return targetIndex >= numberOfLedSegments;
}

bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  const uint16_t numberOfLedSegments = strip.numPixels();
  static uint16_t targetIndex = numberOfLedSegments - 1;
  static unsigned long previousMillis = 0;
  static uint32_t ledStates[LED_COUNT];
  static uint8_t fadeLevel = 0;

  // reset condition
  if (restart)
  {
    targetIndex = numberOfLedSegments - 1;
    previousMillis = 0;
    fadeLevel = 0;

    // save initial state
    for(uint16_t i = 0; i < numberOfLedSegments; ++i)
      ledStates[i] = strip.getPixelColor(i);
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;
  const uint32_t c = color.get_color(targetIndex, numberOfLedSegments);

  const unsigned long currentMillis = millis();
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

  return targetIndex >= numberOfLedSegments;
}

};