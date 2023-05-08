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
  const uint16_t maxCutOff = min(max(cutOff * LED_COUNT, 1), LED_COUNT);
  for(uint16_t i = 0; i < LED_COUNT; ++i)
  {
    if (i >= maxCutOff)
      break;
    
    strip.setPixelColor(i, color.get_color(i, LED_COUNT));
  }
  strip.show();
}


bool dotPingPong(const Color& color, const uint32_t duration, const bool restart, Adafruit_NeoPixel& strip)
{
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    dotWipeUp(color, duration/2, restart, strip);
    dotWipeDown(color, duration/2, restart, strip);
  }

  if (isPongMode)
  {
    return dotWipeUp(color, duration/2, false, strip);
  }
  else {
    // set pong mode to true when first animation is finished
    isPongMode = dotWipeDown(color, duration/2, false, strip);
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

void rainbowFade2White(int wait, int rainbowLoops, Adafruit_NeoPixel& strip) {
  int fadeVal = 0, fadeMax = 100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for (uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops * 65536;
       firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      strip.setPixelColor(i, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(
                                 pixelHue, 255, 255 * fadeVal / fadeMax)));
    }

    strip.show();
    delay(wait);

    if (firstPixelHue < 65536) {                                 // First loop,
      if (fadeVal < fadeMax) fadeVal++;                          // fade in
    } else if (firstPixelHue >= ((rainbowLoops - 1) * 65536)) {  // Last loop,
      if (fadeVal > 0) fadeVal--;                                // fade out
    } else {
      fadeVal = fadeMax;  // Interim loop, make sure fade is at max
    }
  }
}

}