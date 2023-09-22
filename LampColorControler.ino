#include <Adafruit_NeoPixel.h>
#include <cstdlib>
#include "MicroPhone.h"

#include "button.h"
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
  Serial.begin(115200);

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
  pinMode(BUTTON_PIN, INPUT);

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

uint8_t colorState = 0;
uint8_t maxColorInRotation = 9;
bool isFinished = true;
bool switchMode = true;
void incrementColorWheel()
{
  colorState += 1;
  if (colorState > maxColorInRotation)
    colorState = 0;
 
  isFinished = true;
  switchMode = true;
}

void decrement_color_wheel()
{
  colorState -= 1;
  if (colorState > maxColorInRotation)
    colorState = maxColorInRotation;

  isFinished = true;
  switchMode = true;
}

void colorDisplay()
{
  switch(colorState)
  {
    case 0:
      // display a color animation
      static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);  // swirl animation (5 seconds)
      fill(rainbowSwirl, strip);
      rainbowSwirl.update();  // update 
    break;

    case 1:
      // random solid color
      static GenerateRandomColor randomColor = GenerateRandomColor();
      isFinished = doubleSideFillUp(randomColor, 1000, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
    break;

    case 2:
      static GenerateComplementaryColor complColor = GenerateComplementaryColor(0.3);
      isFinished = switchMode ? colorWipeUp(complColor, 1000, isFinished, strip) : colorWipeDown(complColor, 1000, isFinished, strip);
      if (isFinished)
      {
        switchMode = !switchMode;
        complColor.update();  // update color
      }
    break;

    case 3:
      static GenerateGradientColor gradientColor = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
      vu_meter(gradientColor);
    break;

    case 4:
      static GenerateRainbowColor rainbowColor = GenerateRainbowColor();      // will output a rainbow from start to bottom of the display
      isFinished = switchMode ? fadeOut(300, isFinished, strip) : fadeIn(rainbowColor, 1000, isFinished, strip);
      if (isFinished) switchMode = !switchMode;
    break;

    case 5:
      police(400, false, strip);
    break;

    case 6:
      static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(10);     // pulse around a rainbow, with a certain color division
      isFinished = fadeIn(rainbowPulse, 5000, isFinished, strip);
      if (isFinished)
      {
        rainbowPulse.update();
      }
    break;

    case 7:
      // ping pong a color for infinity
      isFinished = dotPingPong(complColor, 1000.0, isFinished, strip);
      if (isFinished) complColor.update();  // update color
    break;

    case 8:
    static GeneratePaletteStep paletteColor = GeneratePaletteStep(PaletteLavaColors);
    if (paletteColor.update()) fill(paletteColor, strip);
    break;

    case 9:
    // wipe a color pulse around the tube at each beat
    if (pulse_beat_wipe(complColor)) complColor.update();
    break;

    default:
    break;
  }
}


bool isActivated = false;
// call when the button is finally release
void buttonCallback(uint8_t consecutiveButtonCheck, uint32_t lastButtonHoldDuration)
{
  if (consecutiveButtonCheck == 0)
    return;

  const bool wasLongPress = lastButtonHoldDuration > 500;
  switch(consecutiveButtonCheck)
  {
    case 0:
      // error
      break;
    
    case 1:
    // 1 press
      if (wasLongPress)
        isActivated = !isActivated;

      if (isActivated)
        incrementColorWheel();
      break;

    case 2:
      if (isActivated)
        decrement_color_wheel();
      break;
    
    default:
    // error
      break;
  }

  if (! wasLongPress)
    Serial.println("Pressed the button " + String(consecutiveButtonCheck) + " times");
  else
    Serial.println("Pressed the button " + String(consecutiveButtonCheck) +  " times, with a long press of " + String(lastButtonHoldDuration) + "ms");
}

void loop() {
  handle_button_events(buttonCallback);

  if (!isActivated)
  {
    strip.clear();
    strip.show();  //  Update strip to match
  }
  else
    colorDisplay();

  
  static GenerateSolidColor blackColor = GenerateSolidColor(0);
  static GenerateRoundColor rdColor = GenerateRoundColor();

  /*static uint32_t timing = 50;
  isFinished = fadeIn(paletteColor, timing, isFinished, strip);
  if (isFinished)
  {
    paletteColor.update();  // update color
    timing = 50 + rand()/(float)RAND_MAX * 50;
  }*/

  //fill(rdColor, strip);

  /*isFinished = colorPulse(rainbowColor, 100, 300, isFinished, strip, 1.0);
  if (isFinished)
    delay(500);*/
}
