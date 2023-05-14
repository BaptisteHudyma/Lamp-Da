#include <Adafruit_NeoPixel.h>
#include <cstdlib>
#include "MicroPhone.h"

#include "animations.h"
#include "wipes.h"
#include "utils.h"


#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN D0

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50  // max: 255

using namespace animations;

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB);

void setup()
{
  Serial.begin(9600);

  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);

  pinMode(LED_BUILTIN, OUTPUT);

  init_microphone(16000);
}


/**
 * \brief beat the color to the music pulse
 * \return true when a beat is detected
 */
bool pulse_beat_wipe(const Color& color)
{
  // reset the pulse for each beat
  static uint32_t durationMillis = 1000 / 6.0;  // max beat period
  const bool beatDetected = get_beat_probability() > 0.9;
  
  // animation is finished and a beat is detected
  if(beatDetected)
  {
    // reset pulse
    doubleSideFillUp(color, durationMillis, true, strip);
    return true;
  }

  doubleSideFillUp(color, durationMillis, false, strip);
  return false;
}

/**
 * \brief Vu meter: should be reactive
 */
void vu_meter(const Color& vuColor)
{
  const float decibels = get_sound_level_Db();
  // convert to 0 - 1
  const float vuLevel = (decibels + abs(silenceLevelDb)) / highLevelDb;

  // display the gradient
  strip.clear();
  fill(vuColor, strip, vuLevel);
}

void loop() {
  static GenerateGradientColor gradientColor = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
  static GenerateComplementaryColor complColor = GenerateComplementaryColor(0.3);
  static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(8);     // pulse around a rainbow, with a certain color division
  static GenerateRainbowColor rainbowColor = GenerateRainbowColor();      // will output a rainbow from start to bottom of the display
  static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);  // swirl animation
  static GeneratePaletteStep paletteColor = GeneratePaletteStep(PaletteForestColors);
  static GenerateRandomColor randomColor = GenerateRandomColor();         // random solid color
  static GenerateSolidColor blackColor = GenerateSolidColor(0);

  static bool isFinished = true;
  static bool switchMode = true;
  const uint duration = 100;
  
  //isFinished = doubleSideFillUp(randomColor, duration, isFinished, strip);
  //if (isFinished) randomColor.update();  // update color

  /*isFinished = switchMode ? colorWipeUp(complColor, duration, isFinished, strip) : colorWipeDown(complColor, duration, isFinished, strip);
  if (isFinished)
  {
    switchMode = !switchMode;
    complColor.update();  // update color
  }*/

  // fill the display with a rainbow color
  // fill(rainbowColor, strip, 1.0);

  // police(400, false, strip);

  // ping pong a color for infinity
  //isFinished = dotPingPong(complColor, duration, isFinished, strip);
  //if (isFinished) complColor.update();  // update color

  // isFinished = switchMode ? fadeOut(300, isFinished, strip) : fadeIn(rainbowColor, 1000, isFinished, strip);
  // if (isFinished) switchMode = !switchMode;

  static uint32_t timing = 10;
  isFinished = fadeIn(paletteColor, timing, isFinished, strip);
  if (isFinished)
  {
    paletteColor.update();  // update color
    timing = 50 + rand()/(float)RAND_MAX * 100;
  }

  // rainbow swirl animation
  //if (paletteColor.update()) fill(paletteColor, strip);

  // wipe a color pulse around the tube at each beat
  //if (pulse_beat_wipe(complColor)) complColor.update();

  //vu_meter(gradientColor);

  /*isFinished = colorPulse(rainbowColor, 100, 300, isFinished, strip, 1.0);
  if (isFinished)
    delay(500);*/
}
