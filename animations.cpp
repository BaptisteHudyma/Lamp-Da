#include "animations.h"

#include "constants.h"
#include "wipes.h"
#include "utils.h"

#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <vector>

namespace animations
{

void fill(const Color& color, Adafruit_NeoPixel& strip, const float cutOff)
{
  const float adaptedCutoff = fmin(fmax(cutOff, 0.0), 1.0);
  const uint16_t maxCutOff = fmin(fmax(adaptedCutoff * LED_COUNT, 1.0), LED_COUNT);
  for(uint16_t i = 0; i < LED_COUNT; ++i)
  {
      const uint32_t c = color.get_color(i, LED_COUNT);
    if (i >= maxCutOff)
    {
      // set a color gradient
      float intPart, fracPart;
      fracPart = modf(adaptedCutoff * LED_COUNT, &intPart);
      // adapt the color to have a nice gradient
      const uint16_t hue = utils::rgb2hue(c);
      strip.setPixelColor(i, Adafruit_NeoPixel::ColorHSV(hue, 255, fracPart * 255));
      break;
    }
    
    strip.setPixelColor(i, c);
  }
  strip.show();
}

bool dotPingPong(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    dotWipeUp(color, duration/2, true, strip);
    dotWipeDown(color, duration/2, true, strip);
    return false;
  }

  if (isPongMode)
  {
    return dotWipeUp(color, duration/2, false, strip, cutOff);
  }
  else {
    // set pong mode to true when first animation is finished
    isPongMode = dotWipeDown(color, duration/2, false, strip, cutOff);
  }

  // finished if the target index is over the led limit
  return false;
}

bool colorPulse(const Color& color, const uint32_t durationPulseUp, const uint32_t durationPulseDown, const bool restart, Adafruit_NeoPixel& strip, const float cutOff)
{
  static GenerateSolidColor blackColor = GenerateSolidColor(0);
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    colorWipeUp(color, durationPulseUp, true, strip);
    colorWipeDown(color, durationPulseDown, true, strip);
    return false;
  }

  if (isPongMode)
  {
    // clear the displayed color with a black (turned of) color, from top to bottom, with a duration proportional to the pulse duration
    return colorWipeDown(blackColor, durationPulseDown * cutOff, false, strip, 1.0);
  }
  else
  {
    // set pong mode to true when first animation is finished
    isPongMode = colorWipeUp(color, durationPulseUp, false, strip, cutOff);
  }

  // finished if the target index is over the led limit
  return false;
}

bool doubleSideFillUp(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  if(restart)
  {
    colorWipeUp(color, duration, true, strip, 0.5);
    colorWipeDown(color, duration, true, strip, 0.5);
    return false;
  }

  return colorWipeDown(color, duration, false, strip, 0.5) or colorWipeUp(color, duration, false, strip, 0.5);
}

bool police(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static unsigned long previousMillis = 0;
  static uint8_t state = 0;

  if (restart)
  {
    previousMillis = 0;
    state = 0;
    return false;
  }

  // convert duration in delay
  const uint16_t delay = duration / 3;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;
  
    switch(state)
    {
      case 0:
      {
        strip.clear();

        // blue lights
        strip.fill(Adafruit_NeoPixel::Color(0, 0, 255), 0, LED_COUNT / 2+1);  // Set blue
        strip.show();  // Update strip with new contents
        
        // set next state
        state = 1;
        break;
      }
      case 1:
      {
        strip.clear();
        strip.fill(Adafruit_NeoPixel::Color(0, 0, 255), LED_COUNT / 2, LED_COUNT);  // Set red
        strip.show();  // Update strip with new contents

        // set next state
        state = 2;
        break;
      }
      case 2:
      default:
      {
        strip.clear();
        strip.show();

        // reset state
        state = 0;
        return true;
      }
    }
  }

  // never ends
  return false;
}

bool fadeOut(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static unsigned long startMillis = 0;
  static uint8_t fadeLevel = 0;
  static uint32_t ledStates[LED_COUNT];

  if (restart)
  {
    fadeLevel = 0;
    startMillis = millis();

    // save initial state
    for(uint16_t i = 0; i < LED_COUNT; ++i)
      ledStates[i] = strip.getPixelColor(i);
    
    return false;
  }
  // out condition: faded to zero
  if(fadeLevel == 255)
    return true;
  
  // get a fade level between 0 and 255
  const uint8_t newFadeLevel = fmax(0.0, fmin(1.0, (millis() - startMillis) / (float)duration)) * 255;
  if (newFadeLevel != fadeLevel)
  {
    fadeLevel = newFadeLevel;
  
    // update all values of rgb
    for(uint16_t i = 0; i < LED_COUNT; ++i)
    {
      const uint32_t startColor = ledStates[i];
      // diminish fade
      strip.setPixelColor(i, utils::get_gradient(startColor, 0, fadeLevel/255.0));
    }
    strip.show();

    if(fadeLevel == 255)
      return true;
  }

  return false;
}

bool fadeIn(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip, const float firstCutOff, const float secondCutOff)
{
  static unsigned long startMillis = 0;
  static const uint16_t maxFadeLevel = 512;
  static uint16_t fadeLevel = 0;
  static uint32_t ledStates[LED_COUNT];

  if (restart)
  {
    fadeLevel = 0;
    startMillis = millis();

    // save initial state
    for(uint16_t i = 0; i < LED_COUNT; ++i)
      ledStates[i] = strip.getPixelColor(i);

    return false;
  }
  // out condition: faded to maximum
  if(fadeLevel >= maxFadeLevel)
    return true;
  
  // get a fade level between 0 and maxFadeLevel
  const uint16_t newFadeLevel = fmax(0.0, fmin(1.0, (millis() - startMillis) / (float)duration)) * maxFadeLevel;
  if (newFadeLevel != fadeLevel)
  {
    fadeLevel = newFadeLevel;

    const uint16_t minIndex = firstCutOff * LED_COUNT;
    const uint16_t maxIndex = secondCutOff * LED_COUNT;
    // update all values of rgb
    for(uint16_t i = minIndex; i < maxIndex; ++i)
    {
      const uint32_t pixelColor = ledStates[i];
      // fade in
      strip.setPixelColor(i, utils::get_gradient(pixelColor, color.get_color(i, LED_COUNT), fadeLevel/(float)maxFadeLevel));
    }
    strip.show();

    if(fadeLevel >= maxFadeLevel)
      return true;
  }

  return false;
}

}