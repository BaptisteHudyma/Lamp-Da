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
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const uint16_t delay = duration / (float)numberOfLedSegments;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex < numberOfLedSegments) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex, color.get_color(targetIndex, numberOfLedSegments, 0));  //  Set pixel's color (in RAM)
    targetIndex += 1;
    strip.show();  //  Update strip to match
  }

  return false;
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
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay and targetIndex >= 0) {
    previousMillis = currentMillis;

    strip.clear();
    strip.setPixelColor(targetIndex--, color.get_color(targetIndex, numberOfLedSegments, 0));  //  Set pixel's color (in RAM)
    strip.show();  //  Update strip to match
  }

  return false;
}

bool colorWipeDown(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip) {
  static uint16_t targetIndex = 0;
  static unsigned long previousMillis = 0;
  static uint32_t nextColor = 0; 
  const uint16_t numberOfLedSegments = strip.numPixels();

  // reset condition
  if (restart)
  {
    targetIndex = 0;
    previousMillis = 0;
    nextColor = 0;
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;
  const uint32_t c = color.get_color(targetIndex, numberOfLedSegments, 0);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;

    strip.setPixelColor(targetIndex++, c);
    nextColor = strip.getPixelColor(targetIndex);
  }
  else
  {
    // update the value of the last segement with a gradient
    const float coeff = (currentMillis - previousMillis) / (float)delay;
    strip.setPixelColor(targetIndex, get_gradient(nextColor, c, coeff));
  }

  strip.show();  //  Update strip to match
  return false;
}

bool colorWipeUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip) {
  const uint16_t numberOfLedSegments = strip.numPixels();
  static uint16_t targetIndex = numberOfLedSegments - 1;
  static unsigned long previousMillis = 0;
  static uint32_t nextColor = 0; 

  // reset condition
  if (restart)
  {
    targetIndex = numberOfLedSegments - 1;
    previousMillis = 0;
    nextColor = 0;
    return false;
  }

  // finished if the target index is over the led limit
  if (targetIndex >= numberOfLedSegments)
    return true;

  // convert duration in delay for each segment
  const unsigned long delay = duration / (float)numberOfLedSegments;
  const uint32_t c = color.get_color(targetIndex, numberOfLedSegments, 0);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;

    strip.setPixelColor(targetIndex--, c);
    nextColor = strip.getPixelColor(targetIndex);
  }
  else
  {
    // update the value of the last segement with a gradient
    const float coeff = (currentMillis - previousMillis) / (float)delay;
    strip.setPixelColor(targetIndex, get_gradient(nextColor, c, coeff));
  }

  strip.show();  //  Update strip to match
  return false;
}

};