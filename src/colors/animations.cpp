#include "animations.h"

#include "../utils/constants.h"
#include "../utils/utils.h"
#include "../utils/coordinates.h"

#include "palettes.h"
#include "wipes.h"

#include <cstdint>
#include <math.h>
#include <cmath>

#include "../ext/math8.h"
#include "../ext/random8.h"
#include "../ext/noise.h"

namespace animations
{

void fill(const Color& color, LedStrip& strip, const float cutOff)
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
      COLOR newC;
      newC.color = c;
      const uint16_t hue = utils::ColorSpace::HSV(newC).get_scaled_hue();
      strip.setPixelColor(i, LedStrip::ColorHSV(hue, 255, fracPart * 255));
      break;
    }
    
    strip.setPixelColor(i, c);
  }
}

bool dot_ping_pong(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float cutOff)
{
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    dot_wipe_up(color, duration/2, true, strip);
    dot_wipe_down(color, duration/2, true, strip);
    return false;
  }

  if (isPongMode)
  {
    return dot_wipe_up(color, duration/2, false, strip, cutOff);
  }
  else {
    // set pong mode to true when first animation is finished
    isPongMode = dot_wipe_down(color, duration/2, false, strip, cutOff);
  }

  // finished if the target index is over the led limit
  return false;
}

bool color_pulse(const Color& color, const uint32_t durationPulseUp, const uint32_t durationPulseDown, const bool restart, LedStrip& strip, const float cutOff)
{
  static GenerateSolidColor blackColor = GenerateSolidColor(0);
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    color_wipe_up(color, durationPulseUp, true, strip);
    color_wipe_down(color, durationPulseDown, true, strip);
    return false;
  }

  if (isPongMode)
  {
    // clear the displayed color with a black (turned of) color, from top to bottom, with a duration proportional to the pulse duration
    return color_wipe_down(blackColor, durationPulseDown * cutOff, false, strip, 1.0);
  }
  else
  {
    // set pong mode to true when first animation is finished
    isPongMode = color_wipe_up(color, durationPulseUp, false, strip, cutOff);
  }

  // finished if the target index is over the led limit
  return false;
}

bool double_side_fill(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip)
{
  if(restart)
  {
    color_wipe_up(color, duration, true, strip, 0.5);
    color_wipe_down(color, duration, true, strip, 0.5);
    return false;
  }

  return color_wipe_down(color, duration, false, strip, 0.5) or color_wipe_up(color, duration, false, strip, 0.5);
}

bool police(const uint32_t duration, const bool restart, LedStrip& strip)
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
  const uint16_t partDelay = duration / 3;

  const uint32_t bluePartLenght = 3.0/5.0 * partDelay;
  const uint32_t clearPartLenght = 2.0/5.0 * bluePartLenght;

  const unsigned long currentMillis = millis();
   

  switch(state)
  {
    // first left blue flash
    case 0:
    {
      strip.clear();

      // blue lights
      strip.fill(LedStrip::Color(0, 0, 255), 0, LED_COUNT / 2+1);  // Set on the first part
      
      // set next state
      state++;
      break;
    }
    // first short clear
    case 1:
    {
      if (currentMillis - previousMillis >= bluePartLenght) {
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
      if (currentMillis - previousMillis >= bluePartLenght) {
        previousMillis = currentMillis;

        strip.clear();

        // blue lights
        strip.fill(LedStrip::Color(0, 0, 255), 0, LED_COUNT / 2+1);  // Set on the first part
        
        // set next state
        state++;
      }
      break;
    }

    // first right blue flash
    case 3:
    {
      if (currentMillis - previousMillis >= clearPartLenght) {
        previousMillis = currentMillis;

        strip.clear();
        strip.fill(LedStrip::Color(0, 0, 255), LED_COUNT / 2, LED_COUNT);  // Set on the second part

        // set next state
        state++;
      }
      break;
    }
    // first short pause
    case 4:
    {
      if (currentMillis - previousMillis >= bluePartLenght) {
        previousMillis = currentMillis;

        strip.clear();
        
        // set next state
        state++;
      }
      break;
    }
    case 5:
    {
      if (currentMillis - previousMillis >= bluePartLenght) {
        previousMillis = currentMillis;

        strip.clear();
        strip.fill(LedStrip::Color(0, 0, 255), LED_COUNT / 2, LED_COUNT);  // Set on the second part

        // set next state
        state++;
      }
      break;
    }
    case 6:
    {
      if (currentMillis - previousMillis >= bluePartLenght) {
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

bool fade_out(const uint32_t duration, const bool restart, LedStrip& strip)
{
  static unsigned long startMillis = 0;
  static uint32_t maxFadeLevel = 0;
  static uint32_t fadeLevel = 0;

  // shared buffer
  static uint32_t* ledStates = strip.get_buffer_ptr(0);

  if (restart)
  {
    fadeLevel = 0;
    maxFadeLevel = duration / LOOP_UPDATE_PERIOD;
    startMillis = millis();

    // buffer the start values
    strip.buffer_current_colors(0);
    
    return false;
  }
  
  // get a fade level between 0 and max level
  const uint8_t newFadeLevel = fmax(0.0, fmin(1.0, (millis() - startMillis) / (float)duration)) * maxFadeLevel;
  if (newFadeLevel != fadeLevel)
  {
    fadeLevel = newFadeLevel;
  
    // update all values of rgb
    for(uint16_t i = 0; i < LED_COUNT; ++i)
    {
      const uint32_t startColor = ledStates[i];
      // diminish fade
      strip.setPixelColor(i, utils::get_gradient(startColor, 0, fadeLevel/float(maxFadeLevel)));
    }
  }

  if(fadeLevel >= maxFadeLevel)
      return true;

  return false;
}

bool fade_in(const Color& color, const uint32_t duration, const bool restart, LedStrip& strip, const float firstCutOff, const float secondCutOff)
{
  static unsigned long startMillis = 0;
  static uint32_t maxFadeLevel = 0;
  static uint32_t fadeLevel = 0;
  static uint32_t* ledStates = strip.get_buffer_ptr(0);
  static uint32_t* targetStates = strip.get_buffer_ptr(1);

  if (restart)
  {
    fadeLevel = 0;
    maxFadeLevel = duration / LOOP_UPDATE_PERIOD;
    startMillis = millis();

    // buffer the start values
    strip.buffer_current_colors(0);

    // save initial state
    for(uint16_t i = 0; i < LED_COUNT; ++i)
    {
      targetStates[i] = color.get_color(i, LED_COUNT);
    }

    return false;
  }
  
  // get a fade level between 0 and maxFadeLevel
  const uint32_t newFadeLevel = fmax(0.0, fmin(1.0, (millis() - startMillis) / (float)duration)) * maxFadeLevel;
  if (newFadeLevel != fadeLevel)
  {
    fadeLevel = newFadeLevel;

    const uint16_t minIndex = firstCutOff * LED_COUNT;
    const uint16_t maxIndex = secondCutOff * LED_COUNT;
    // update all values of rgb
    for(uint16_t i = minIndex; i < maxIndex; ++i)
    {
      // fade in
      strip.setPixelColor(i, utils::get_gradient(ledStates[i], targetStates[i], fadeLevel/(float)maxFadeLevel));
    }
  }
  if(fadeLevel >= maxFadeLevel)
    return true;

  return false;
}

bool fire(const bool isFirstCall, LedStrip& strip)
{
  constexpr bool gReverseDirection = true;
  
  random16_add_entropy( random());

  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  // Default 50, suggested range 20-100 
  constexpr uint8_t COOLING = 70;

  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  // Default 120, suggested range 50-200.
  constexpr uint8_t SPARKING = 200;

  // Array of temperature readings at each simulation cell
  static uint8_t* heat = strip._buffer8b;

  if(isFirstCall)
  {
    // reset the values of the buffer
    memset(strip._buffer8b, 0, sizeof(strip._buffer8b));
    strip.clear();
  }

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < LED_COUNT; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / LED_COUNT) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for(int j = 0; j < LED_COUNT; j++) {
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (LED_COUNT-1) - j;
    } else {
      pixelnumber = j;
    }

    uint8_t colorindex = scale8( heat[j], 240);
    strip.setPixelColor(pixelnumber, get_color_from_palette(colorindex, PaletteHeatColors));
  }

  return true;
}


void random_noise(const palette_t& palette, LedStrip& strip, const bool restart, const bool isColorLoop, const uint16_t scale)
{
  static const float speed = 3000;

  static uint16_t* noise = strip._buffer16b;
  // noise coordinates
  static float x = random16();
  static float y = random16();
  static float z = random16();

  if(restart)
  {
    x = random16();
    y = random16();
    z = random16();
    memset(strip._buffer16b, 0, sizeof(strip._buffer16b));
  }

  static uint32_t lastcall = 0;
  if(millis() - lastcall > 30)
  {
    lastcall = millis();

    // how much the new value influences the last one
    float dataSmoothing = 0.01;
    for(int i = 0; i < LED_COUNT; i++) {
      const auto res = to_lamp(i);
      uint16_t data = noise16::inoise(x + scale * res.x, y + scale * res.y, z + scale * res.z);

      // smooth over time to prevent suddent jumps
      noise[i] = noise[i] * dataSmoothing +  data * (1.0 - dataSmoothing);
    }

    // apply slow drift to X and Y, just for visual variation.
    x += speed / 8;
    y -= speed / 16;

    static uint16_t ihue = 0;
    for(int i = 0; i < LED_COUNT; i++) {
      uint16_t index = noise[i];
      uint8_t bri = noise[LED_COUNT - 1 - i] >> 8;

      // if this palette is a 'loop', add a slowly-changing base value
      if(isColorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      strip.setPixelColor(i, get_color_from_palette(index, palette, bri));
    }
    
    ihue+=1;
  }
}

void candle(const palette_t& palette, LedStrip& strip)
{
  constexpr uint32_t minPhaseDuration = 1000; 
  constexpr uint32_t maxPhaseDuration = 5000;

  constexpr uint32_t sequenceDividerPeriod = 10;  // milliseconds for each sequence phase

  constexpr uint8_t flickeringMin = 0;
  constexpr uint8_t flickeringMax = 100;

  static uint32_t phaseDuration = minPhaseDuration;
  static uint32_t phaseStartTime = 0;
  static uint32_t targetPhaseAmplitude = 0;
  static uint32_t targetPhaseStrength = 0;

  static float noisePosition = 0.0; 
  constexpr float noiseSpeed = 5; 
  constexpr float noiseScale = 10;

  static uint32_t sequenceTime = 0;
  static uint32_t sequenceIndex = 0;
  static uint32_t sequencePerPhase = 0;
  static uint8_t lastStrenght = 100;
  static uint8_t currentStrenght = 100;

  // phase is a flicker phase: last a few seconds during witch a flickering sequence will happen
  const uint32_t currentMillis = millis();
  if (currentMillis - phaseStartTime > phaseDuration || sequenceIndex > sequencePerPhase)
  {
    // declare a new phase
    phaseStartTime = millis();
    // set flame parameters
    targetPhaseAmplitude = random(flickeringMin, flickeringMax) + 1;
    targetPhaseStrength = random(targetPhaseAmplitude / 4, 255);
    phaseDuration = random(minPhaseDuration, maxPhaseDuration);
    lastStrenght = currentStrenght;

    sequencePerPhase = phaseDuration / sequenceDividerPeriod;
    sequenceTime = 0; // start a new sequence
    sequenceIndex = 0;
  }

  if (currentMillis - sequenceTime > sequenceDividerPeriod)
  {
    // sequence update
    sequenceTime = currentMillis;

    const float endStrengthRatio = (double)sequenceIndex/(double)sequencePerPhase;
    const float oldStrengthRatio = 1.0 - endStrengthRatio;

    currentStrenght = (lastStrenght * oldStrengthRatio) + (targetPhaseStrength * endStrengthRatio);

    const uint8_t minBrighness = currentStrenght - min(currentStrenght, targetPhaseAmplitude / 2);
    const uint8_t maxBrighness = currentStrenght + min(255 - currentStrenght, targetPhaseAmplitude / 2);

    uint8_t brightness = random(minBrighness, maxBrighness);
    for(uint16_t i = 0; i < LED_COUNT; ++i)
    {
      // vary flicker wiht bigger amplitude
      if (i % 4 == 0)
        brightness = random(minBrighness, maxBrighness);
      strip.setPixelColor(i, get_color_from_palette(noise8::inoise(noisePosition + noiseScale * i), palette, brightness));
    }

    // faster speed when change is fast
    noisePosition += noiseSpeed;

    // ready for new sequence
    sequenceIndex += 1;
  }
}

}