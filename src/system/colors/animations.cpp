#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "animations.h"

#include <math.h>
#include <sys/types.h>

#include <cmath>
#include <cstdint>

#include "src/system/colors/palettes.h"
#include "src/system/colors/wipes.h"
#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"
#include "src/system/ext/random8.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/coordinates.h"
#include "src/system/utils/utils.h"

namespace animations {

void fill(const Color& color, LedStrip& strip, const float cutOff)
{
  const float adaptedCutoff = fmin(fmax(cutOff, 0.0), 1.0);
  const uint16_t maxCutOff = fmin(fmax(adaptedCutoff * LED_COUNT, 1.0), LED_COUNT);
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

bool color_pulse(const Color& color,
                 const uint32_t durationPulseUp,
                 const uint32_t durationPulseDown,
                 const bool restart,
                 LedStrip& strip,
                 const float cutOff)
{
  static GenerateSolidColor blackColor = GenerateSolidColor(0);
  static bool isPongMode = false; // true: animation is climbing back the display

  // reset condition
  if (restart)
  {
    isPongMode = false;
    color_wipe_up(color, durationPulseUp, true, strip);
    color_wipe_down(color, durationPulseDown, true, strip);
  }

  if (isPongMode)
  {
    // clear the displayed color with a black (turned of) color, from top to
    // bottom, with a duration proportional to the pulse duration
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

  const unsigned long currentMillis = millis();

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
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      const uint32_t startColor = ledStates[i];
      // diminish fade
      strip.setPixelColor(i, utils::get_gradient(startColor, 0, fadeLevel / float(maxFadeLevel)));
    }
  }

  if (fadeLevel >= maxFadeLevel)
    return true;

  return false;
}

bool fade_in(const Color& color,
             const uint32_t duration,
             const bool restart,
             LedStrip& strip,
             const float firstCutOff,
             const float secondCutOff)
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
    for (uint16_t i = 0; i < LED_COUNT; ++i)
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
    for (uint16_t i = minIndex; i < maxIndex; ++i)
    {
      // fade in
      strip.setPixelColor(i, utils::get_gradient(ledStates[i], targetStates[i], fadeLevel / (float)maxFadeLevel));
    }
  }
  if (fadeLevel >= maxFadeLevel)
    return true;

  return false;
}

void fire(const uint8_t scalex, const uint8_t scaley, const uint8_t speed, const palette_t& palette, LedStrip& strip)
{
  static uint32_t step = 0;
  uint16_t zSpeed = step / (256 - speed);
  uint16_t ySpeed = millis() / (256 - speed);
  for (int i = 0; i < ceil(stripXCoordinates); i++)
  {
    for (int j = 0; j < ceil(stripYCoordinates); j++)
    {
      strip.setPixelColorXY(
              stripXCoordinates - i,
              j,
              get_color_from_palette(qsub8(noise8::inoise(i * scalex, j * scaley + ySpeed, zSpeed),
                                           abs8(j - (stripXCoordinates - 1)) * 255 / (stripXCoordinates - 1)),
                                     palette));
    }
  }
  step++;
}

void random_noise(
        const palette_t& palette, LedStrip& strip, const bool restart, const bool isColorLoop, const uint16_t scale)
{
  static const float speed = 3000;

  static uint16_t* noise = strip._buffer16b;
  // noise coordinates
  static float x = random16();
  static float y = random16();
  static float z = random16();

  if (restart)
  {
    x = random16();
    y = random16();
    z = random16();
    memset(strip._buffer16b, 0, sizeof(strip._buffer16b));
  }

  static uint32_t lastcall = 0;
  if (millis() - lastcall > 30)
  {
    lastcall = millis();

    // how much the new value influences the last one
    float dataSmoothing = 0.01;
    for (int i = 0; i < LED_COUNT; i++)
    {
      const auto res = strip.get_lamp_coordinates(i);
      uint16_t data = noise16::inoise(x + scale * res.x, y + scale * res.y, z + scale * res.z);

      // smooth over time to prevent suddent jumps
      noise[i] = noise[i] * dataSmoothing + data * (1.0 - dataSmoothing);
    }

    // apply slow drift to X and Y, just for visual variation.
    x += speed / 8;
    y -= speed / 16;

    static uint16_t ihue = 0;
    for (int i = 0; i < LED_COUNT; i++)
    {
      uint16_t index = noise[i];
      uint8_t bri = noise[LED_COUNT - 1 - i] >> 8;

      // if this palette is a 'loop', add a slowly-changing base value
      if (isColorLoop)
      {
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if (bri > 127)
      {
        bri = 255;
      }
      else
      {
        bri = dim8_raw(bri * 2);
      }

      strip.setPixelColor(i, get_color_from_palette(index, palette, bri));
    }

    ihue += 1;
  }
}

void candle(const palette_t& palette, LedStrip& strip)
{
  constexpr uint32_t minPhaseDuration = 1000;
  constexpr uint32_t maxPhaseDuration = 5000;

  constexpr uint32_t sequenceDividerPeriod = 10; // milliseconds for each sequence phase

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

  // phase is a flicker phase: last a few seconds during witch a flickering
  // sequence will happen
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

    const float endStrengthRatio = (double)sequenceIndex / (double)sequencePerPhase;
    const float oldStrengthRatio = 1.0 - endStrengthRatio;

    currentStrenght = (lastStrenght * oldStrengthRatio) + (targetPhaseStrength * endStrengthRatio);

    const uint8_t minBrighness = currentStrenght - min(currentStrenght, targetPhaseAmplitude / 2);
    const uint8_t maxBrighness = currentStrenght + min(UINT8_MAX - currentStrenght, targetPhaseAmplitude / 2);

    uint8_t brightness = random(minBrighness, maxBrighness);
    for (uint16_t i = 0; i < LED_COUNT; ++i)
    {
      // vary flicker wiht bigger amplitude
      if (i % 4 == 0)
        brightness = random(minBrighness, maxBrighness);
      strip.setPixelColor(i,
                          get_color_from_palette(noise8::inoise(noisePosition + noiseScale * i), palette, brightness));
    }

    // faster speed when change is fast
    noisePosition += noiseSpeed;

    // ready for new sequence
    sequenceIndex += 1;
  }
}

void phases(const bool moder, const uint8_t speed, const palette_t& palette, LedStrip& strip)
{ // We're making sine waves here. By Andrew Tuline.
  static uint32_t step = 0;

  uint8_t allfreq = 16;                           // Base frequency.
  float* phase = reinterpret_cast<float*>(&step); // Phase change value gets calculated
                                                  // (float fits into unsigned long).
  uint8_t cutOff = (255 - 128);                   // You can change the number of pixels.  AKA INTENSITY (was 192).
  uint8_t modVal = 5;                             // SEGMENT.fft1/8+1;                         // You can
                                                  // change the modulus. AKA FFT1 (was 5).

  uint8_t index = millis() / 64; // Set color rotation speed
  *phase += speed / 32.0;        // You can change the speed of the wave. AKA SPEED (was .4)

  for (int i = 0; i < LED_COUNT; i++)
  {
    if (moder)
      modVal = (noise8::inoise(i * 10 + i * 10) / 16); // Let's randomize our mod length with some Perlin noise.
    uint16_t val = (i + 1) * allfreq;                  // This sets the frequency of the waves.
                                                       // The +1 makes sure that led 0 is used.
    if (modVal == 0)
      modVal = 1;
    val += *phase * (i % modVal + 1) / 2; // This sets the varying phase change
                                          // of the waves. By Andrew Tuline.
    uint8_t b = cubicwave8(val);          // Now we make an 8 bit sinewave.
    b = (b > cutOff) ? (b - cutOff) : 0;  // A ternary operator to cutoff the light.

    COLOR c;
    c.color = get_color_from_palette(index, palette);

    COLOR cFond;
    cFond.color = strip.getPixelColor(i);
    strip.setPixelColor(i, utils::color_blend(cFond, c, b));
    index += 256 / LED_COUNT;
    if (LED_COUNT > 256)
      index++; // Correction for segments longer than 256 LEDs
  }
}

void mode_2DPolarLights(
        const uint8_t scale, const uint8_t speed, const palette_t& palette, const bool reset, LedStrip& strip)
{ // By: Kostyantyn Matviyevskyy
  // https://editor.soulmatelights.com/gallery/762-polar-lights
  // , Modified by: Andrew Tuline
  const uint16_t cols = stripXCoordinates;
  const uint16_t rows = stripYCoordinates;
  static uint32_t step = 0;

  if (reset)
  {
    strip.clear();
    step = 0;
  }

  float adjustHeight = (float)map(rows, 8, 32, 28, 12); // maybe use mapf() ???
  uint16_t adjScale = map(cols, 8, 64, 310, 63);
  /*
    if (SEGENV.aux1 != SEGMENT.custom1/12) {   // Hacky palette rotation. We
    need that black. SEGENV.aux1 = SEGMENT.custom1/12; for (int i = 0; i < 16;
    i++) { long ilk; ilk = (long)currentPalette[i].r << 16; ilk +=
    (long)currentPalette[i].g << 8; ilk += (long)currentPalette[i].b; ilk = (ilk
    << SEGENV.aux1) | (ilk >> (24 - SEGENV.aux1)); currentPalette[i].r = ilk >>
    16; currentPalette[i].g = ilk >> 8; currentPalette[i].b = ilk;
      }
    }
  */
  uint16_t _scale = map(scale, 0, 255, 30, adjScale);
  byte _speed = map(speed, 0, 255, 128, 16);

  for (int x = 0; x <= cols; x++)
  {
    for (int y = 0; y < rows; y++)
    {
      step++;
      strip.setPixelColorXY(
              x,
              y,
              get_color_from_palette(qsub8(noise8::inoise((step % 2) + x * _scale, y * 16 + step % 16, step / _speed),
                                           fabsf((float)rows / 2.0f - (float)y) * adjustHeight),
                                     palette));
    }
  }
}

void mode_2DDrift(const uint8_t intensity, const uint8_t speed, const palette_t& palette, LedStrip& strip)
{ // By: Stepko
  // https://editor.soulmatelights.com/gallery/884-drift
  // , Modified by: Andrew Tuline
  const uint16_t cols = ceil(stripXCoordinates);
  const uint16_t rows = ceil(stripYCoordinates);

  const uint16_t colsCenter = (cols >> 1) + cols % 2;
  const uint16_t rowsCenter = (rows >> 1) + rows % 2;

  strip.fadeToBlackBy(128);
  const uint16_t maxDim = MAX(cols, rows) / 2;
  unsigned long t = millis() / (32 - (speed >> 3));
  unsigned long t_20 = t / 20; // softhack007: pre-calculating this gives about 10% speedup
  for (float i = 1; i < maxDim; i += 0.25)
  {
    float angle = radians(t * (maxDim - i));
    uint16_t myX = colsCenter + (sin_t(angle) * i);
    uint16_t myY = rowsCenter + (cos_t(angle) * i);

    strip.setPixelColorXY(myX, myY, get_color_from_palette((uint8_t)((i * 20) + t_20), palette));
  }
  strip.blur(intensity >> 3);
} // mode_2DDrift()

static const uint8_t exp_gamma[256] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,
        3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,
        9,   9,   10,  10,  10,  11,  11,  12,  12,  12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,
        18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,
        32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  44,  44,  45,  46,  47,  48,  49,
        50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  70,  71,  72,
        73,  74,  75,  77,  78,  79,  80,  82,  83,  84,  85,  87,  89,  91,  92,  93,  95,  96,  98,  99,  100, 101,
        102, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 125, 126, 128, 130, 131, 133, 135, 136,
        138, 140, 142, 143, 145, 147, 149, 151, 152, 154, 156, 158, 160, 162, 164, 165, 167, 169, 171, 173, 175, 177,
        179, 181, 183, 185, 187, 190, 192, 194, 196, 198, 200, 202, 204, 207, 209, 211, 213, 216, 218, 220, 222, 225,
        227, 229, 232, 234, 236, 239, 241, 244, 246, 249, 251, 253, 254, 255};

void hiphotic(const uint8_t speed, LedStrip& strip)
{
  int a = millis() / (32 - (speed >> 3));
  for (int x = 0; x < ceil(stripXCoordinates); x++)
  {
    for (int y = 0; y < ceil(stripYCoordinates); y++)
    {
      COLOR c;
      c.blue = exp_gamma[sin8((x - 8) * cos8((y + 20) * 4) / 4 + a)];
      c.green = exp_gamma[(sin8(x * 16 + a / 3) + cos8(y * 8 + a / 2)) / 2];
      c.red = exp_gamma[sin8(cos8(x * 8 + a / 3) + sin8(y * 8 + a / 4) + a)];

      strip.setPixelColorXY(x, y, c);
    }
  }
}

// Distortion waves - ldirko
// https://editor.soulmatelights.com/gallery/1089-distorsion-waves
// adapted for WLED by @blazoncek
void mode_2Ddistortionwaves(const uint8_t scale, const uint8_t speed, LedStrip& strip)
{
  const uint16_t cols = ceil(stripXCoordinates);
  const uint16_t rows = ceil(stripYCoordinates);

  uint8_t _speed = speed / 32;
  uint8_t _scale = scale / 32;

  uint8_t w = 2;

  uint16_t a = millis() / 32;
  uint16_t a2 = a / 2;
  uint16_t a3 = a / 3;

  uint16_t cx = beatsin8(10 - _speed, 0, cols - 1) * _scale;
  uint16_t cy = beatsin8(12 - _speed, 0, rows - 1) * _scale;
  uint16_t cx1 = beatsin8(13 - _speed, 0, cols - 1) * _scale;
  uint16_t cy1 = beatsin8(15 - _speed, 0, rows - 1) * _scale;
  uint16_t cx2 = beatsin8(17 - _speed, 0, cols - 1) * _scale;
  uint16_t cy2 = beatsin8(14 - _speed, 0, rows - 1) * _scale;

  uint16_t xoffs = 0;
  for (int x = 0; x < cols; x++)
  {
    xoffs += _scale;
    uint16_t yoffs = 0;

    for (int y = 0; y < rows; y++)
    {
      yoffs += _scale;

      byte rdistort = cos8((cos8(((x << 3) + a) & 255) + cos8(((y << 3) - a2) & 255) + a3) & 255) >> 1;
      byte gdistort = cos8((cos8(((x << 3) - a2) & 255) + cos8(((y << 3) + a3) & 255) + a + 32) & 255) >> 1;
      byte bdistort = cos8((cos8(((x << 3) + a3) & 255) + cos8(((y << 3) - a) & 255) + a2 + 64) & 255) >> 1;

      COLOR c;
      c.red = rdistort + w * (a - (((xoffs - cx) * (xoffs - cx) + (yoffs - cy) * (yoffs - cy)) >> 7));
      c.green = gdistort + w * (a2 - (((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1)) >> 7));
      c.blue = bdistort + w * (a3 - (((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2)) >> 7));

      c.red = utils::gamma8(cos8(c.red));
      c.green = utils::gamma8(cos8(c.green));
      c.blue = utils::gamma8(cos8(c.blue));

      strip.setPixelColorXY(x, y, c);
    }
  }
}

// Calm effect, like a lake at night
void mode_lake(const uint8_t speed, const palette_t& palette, LedStrip& strip)
{
  uint8_t sp = speed / 10;
  int wave1 = beatsin8(sp + 2, -64, 64);
  int wave2 = beatsin8(sp + 1, -64, 64);
  uint8_t wave3 = beatsin8(sp + 2, 0, 80);
  // CRGB fastled_col;

  for (int i = 0; i < LED_COUNT; i++)
  {
    uint8_t index = cos8((i * 15) + wave1) / 2 + cubicwave8((i * 23) + wave2) / 2;
    uint8_t lum = (index > wave3) ? index - wave3 : 0;
    // fastled_col = ColorFromPalette(SEGPALETTE, map(index,0,255,0,240), lum,
    // LINEARBLEND); SEGMENT.setPixelColor(i, fastled_col.red,
    // fastled_col.green, fastled_col.blue);
    strip.setPixelColor(i, get_color_from_palette(index, palette, lum));
  }
}

void mode_sinewave(const uint8_t speed, const uint8_t intensity, const palette_t& palette, LedStrip& strip)
{
  // Adjustable sinewave. By Andrew Tuline
  // #define qsuba(x, b)  ((x>b)?x-b:0)               // Analog Unsigned
  // subtraction macro. if result <0, then => 0

  static uint16_t step = 0;

  uint16_t colorIndex = millis() / 32; //(256 - SEGMENT.fft1);  // Amount of colour change.

  step += speed / 16;            // Speed of animation.
  uint16_t freq = intensity / 4; // SEGMENT.fft2/8;                       // Frequency of the signal.

  for (int i = 0; i < LED_COUNT; i++)
  {                                                 // For each of the LED's in the strand, set a brightness based on
                                                    // a wave as follows:
    uint8_t pixBri = cubicwave8((i * freq) + step); // qsuba(cubicwave8((i*freq)+SEGENV.step),
                                                    // (255-SEGMENT.intensity)); // qsub sets a minimum value
                                                    // called thiscutoff. If < thiscutoff, then bright = 0.
                                                    // Otherwise, bright = 128 (as defined in qsub)..
    // setPixCol(i, i*colorIndex/255, pixBri);
    COLOR pixColor;
    pixColor.color = get_color_from_palette((uint8_t)(i * colorIndex / 255), palette);
    COLOR back;
    back.color = 0;
    strip.setPixelColor(i, utils::color_blend(back, pixColor, pixBri));
  }
}

// effect utility functions
uint8_t sin_gap(uint16_t in)
{
  if (in & 0x100)
    return 0;
  return sin8(in + 192); // correct phase shift of sine so that it starts and stops at 0
}

void running_base(
        bool saw, bool dual, const uint8_t speed, const uint8_t intensity, const palette_t& palette, LedStrip& strip)
{
  uint8_t x_scale = intensity >> 2;
  uint32_t counter = (millis() * speed) >> 9;

  COLOR c1;
  c1.color = 0;

  COLOR c2;

  for (int i = 0; i < LED_COUNT; i++)
  {
    uint16_t a = i * x_scale - counter;
    if (saw)
    {
      a &= 0xFF;
      if (a < 16)
      {
        a = 192 + a * 8;
      }
      else
      {
        a = map(a, 16, 255, 64, 192);
      }
      a = 255 - a;
    }
    uint8_t s = dual ? sin_gap(a) : sin8(a);

    c2.color = get_color_from_palette((uint8_t)i, palette);
    COLOR ca = utils::color_blend(c1, c2, s);
    if (dual)
    {
      uint16_t b = (LED_COUNT - 1 - i) * x_scale - counter;
      uint8_t t = sin_gap(b);

      c2.color = get_color_from_palette((uint8_t)i, palette);

      COLOR cb;
      cb = utils::color_blend(c1, c2, t);
      ca = utils::color_blend(ca, cb, 127);
    }
    strip.setPixelColor(i, ca);
  }
}

} // namespace animations

#endif
