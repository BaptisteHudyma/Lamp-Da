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

  static GenerateRainbowColor rainbowColor = GenerateRainbowColor();  // will output a rainbow from start to bottom of the display
  static GenerateGradientColor gradientColor = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
  static GenerateSolidColor color = GenerateSolidColor(utils::get_random_color());
  static GenerateSolidColor altColor = 0;

  static bool isFinished = true;
  static bool switchMode = true;
  const uint duration = 1000;

  colorWipeDown(color, duration, isFinished, strip, 0.5);
  isFinished = colorWipeUp(color, duration, isFinished, strip, 0.5);
  if (isFinished)
  {
    altColor = color;
    color = GenerateSolidColor(utils::get_random_complementary_color(color.get_color(), 0.3));
  }

  // fill the display with a rainbow color
  // fill(rainbowColor, strip, 1.0);

  // police(1000, false, strip);

  // rainbowFade2White(10, 2, strip);

  // ping pong a color for infinity
  // isFinished = dotPingPong(rainbowColor, duration, isFinished, strip);
  // isFinished = dotWipeDown(rainbowColor, duration/2, isFinished, strip);

  /*isFinished = switchMode ? fadeOut(1000, isFinished, strip) : fadeIn(rainbowColor, 1000, isFinished, strip);
  if (isFinished)
  {
    switchMode = !switchMode;
  }*/
}
