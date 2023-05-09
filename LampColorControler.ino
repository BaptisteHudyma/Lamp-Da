#include <Adafruit_NeoPixel.h>
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


void rainbow_pulse_beat()
{
  static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(8);  // pulse around a rainbow, with a certain color division
  static bool isAnimationFinished = true;

  // reset the pulse for each beat
  const bool beatDetected = get_beat_probability() > 0.5;
  
  // animation is finished and a beat is detected
  if(beatDetected)
  {
    fill(rainbowPulse, strip);
    rainbowPulse.update();

    // restart animation
    /*strip.clear();
    strip.show();

    // animation is finished, update the color 
    rainbowPulse.update();
    isAnimationFinished = colorWipeUp(rainbowPulse, 1000/5, true, strip);*/
  }

  //isAnimationFinished = colorWipeUp(rainbowPulse, 1000/5, false, strip);
}


void loop() {
  static GenerateRainbowColor rainbowColor = GenerateRainbowColor();  // will output a rainbow from start to bottom of the display
  static GenerateGradientColor gradientColor = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
  static GenerateSolidColor blackColor = GenerateSolidColor(0);
  static GenerateSolidColor color = GenerateSolidColor(utils::get_random_color());  // random solid color
  static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);            // swirl animation  

  static bool isFinished = true;
  static bool switchMode = true;
  const uint duration = 1000;
  
  /*colorWipeDown(color, duration, isFinished, strip, 0.5);
  isFinished = colorWipeUp(color, duration, isFinished, strip, 0.5);
  if (isFinished)
  {
    color = GenerateSolidColor(utils::get_random_complementary_color(color.get_color(), 0.3));
  }*/

  /*
  isFinished = switchMode ? colorWipeUp(color, duration, isFinished, strip) : colorWipeDown(color, duration, isFinished, strip);
  if (isFinished)
  {
    switchMode = !switchMode;
    color = GenerateSolidColor(utils::get_random_complementary_color(color.get_color(), 0.3));
  }*/

  // fill the display with a rainbow color
  // fill(rainbowColor, strip, 1.0);

  // police(1000, false, strip);

  // ping pong a color for infinity
  // isFinished = dotPingPong(rainbowColor, duration, isFinished, strip);

  /*isFinished = switchMode ? fadeOut(300, isFinished, strip) : fadeIn(rainbowColor, 1000, isFinished, strip);
  if (isFinished)
  {
    switchMode = !switchMode;
  }*/

  // rainbow swirl animation
  // if (rainbowSwirl.update()) (rainbowSwirl, strip);

  rainbow_pulse_beat();
}
