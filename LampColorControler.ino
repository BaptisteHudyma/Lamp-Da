// NeoPixel test program showing use of the WHITE channel for RGBW
// pixels only (won't look correct on regular RGB NeoPixel strips).

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#include "animations.h"
#include "wipes.h"
#include "utils.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN D0

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 29

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50  // max: 255

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
}

void loop() {
  using namespace animations;

  static bool isFinished = false;
    
  //isFinished = dotWipeDownRainbow(1000, isFinished, strip);


  static bool isFadeFinished = false;
  static GenerateSolidColor color = GenerateSolidColor(get_random_color());
  static bool Switch = false;
  const uint dur = 1000;

  isFinished = Switch ? colorWipeUp(color, dur, isFinished, strip) : colorWipeDown(color, dur, isFinished, strip);
  //isFadeFinished = fadeOut(dur, isFinished, strip);
  if (isFinished)
  {
    Switch = !Switch;
    color = GenerateSolidColor(get_random_complementary_color(color.get_color(), 0.3));
  }

  //fill_gradient(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0), strip);

  //police(1000, false, strip);

  //rainbowFade2White(10, 2, strip);
}
