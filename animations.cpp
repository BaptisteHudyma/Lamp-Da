#include "animations.h"

#include "constants.h"
#include "wipes.h"
#include "utils.h"

#include <stdlib.h>
#include <math.h>
#include <vector>

namespace animations
{

void fill(const Color& color, Adafruit_NeoPixel& strip, const float cutOff)
{
  const uint16_t maxCutOff = fmin(fmax(cutOff * LED_COUNT, 1.0), LED_COUNT);
  for(uint16_t i = 0; i < LED_COUNT; ++i)
  {
    if (i >= maxCutOff)
      break;
    
    strip.setPixelColor(i, color.get_color(i, LED_COUNT));
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
    return colorWipeUp(color, durationPulseUp, false, strip, cutOff);
  }
  else
  {
    // set pong mode to true when first animation is finished
    isPongMode = colorWipeDown(blackColor, durationPulseDown * cutOff, false, strip, 1.0);
  }

  // finished if the target index is over the led limit
  return false;
}


bool police(const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static unsigned long previousMillis = 0;
  static bool isBluePhase = false;

  if (restart)
  {
    previousMillis = 0;
    isBluePhase = false;
    return false;
  }

  // convert duration in delay
  const uint16_t delay = duration / 2;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay) {
    previousMillis = currentMillis;
  
    if (isBluePhase)
    {
      strip.clear();

      // blue lights
      strip.fill(Adafruit_NeoPixel::Color(0, 0, 255), 0, LED_COUNT / 2+1);  // Set blue
      strip.show();  // Update strip with new contents
      isBluePhase = false;
    }
    else {
      strip.clear();
      strip.fill(Adafruit_NeoPixel::Color(255, 0, 0), LED_COUNT / 2, LED_COUNT);  // Set red
      strip.show();  // Update strip with new contents

      isBluePhase = true;
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

bool fadeIn(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
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
  // out condition: faded to maximum
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
      const uint32_t pixelColor = ledStates[i];
      // fade in
      strip.setPixelColor(i, utils::get_gradient(pixelColor, color.get_color(i, LED_COUNT), fadeLevel/255.0));
    }
    strip.show();

    if(fadeLevel == 255)
      return true;
  }

  return false;
}

}