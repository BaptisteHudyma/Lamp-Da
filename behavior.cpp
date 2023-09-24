#include "behavior.h"

#include "MicroPhone.h"

#include "button.h"
#include "animations.h"
#include "wipes.h"
#include "utils.h"


// extern declarations
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB);

uint8_t BRIGHTNESS;

bool isActivated = false;


// constantes
constexpr uint8_t MIN_BRIGHTNESS = 15;
constexpr uint8_t MAX_BRIGHTNESS = 255;



uint8_t colorState = 0;
uint8_t maxColorInRotation = 11;
bool isFinished = true;
bool switchMode = true;

void incrementColorWheel()
{
  colorState += 1;
  if (colorState > maxColorInRotation)
    colorState = 0;

  strip.clear();
  strip.show();  //  Update strip to match
  isFinished = true;
  switchMode = false;
}

void decrement_color_wheel()
{
  colorState -= 1;
  if (colorState > maxColorInRotation)
    colorState = maxColorInRotation;

  strip.clear();
  strip.show();  //  Update strip to match
  isFinished = true;
  switchMode = false;
}

void colorDisplay()
{
  static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);  // swirl animation (5 seconds)
  static GenerateRandomColor randomColor = GenerateRandomColor();
  static GenerateComplementaryColor complColor = GenerateComplementaryColor(0.3);
  static GenerateGradientColor gradientColor = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
  static GenerateRainbowColor rainbowColor = GenerateRainbowColor();      // will output a rainbow from start to bottom of the display
  static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(10);     // pulse around a rainbow, with a certain color division
  static GeneratePaletteStep paletteColor = GeneratePaletteStep(PaletteLavaColors);
  static GenerateRoundColor rdColor = GenerateRoundColor();
  static uint32_t timing = 50;
  
  switch(colorState)
  {
    case 0:
      // display a color animation
      animations::fill(rainbowSwirl, strip);
      rainbowSwirl.update();  // update 
    break;

    case 1:
      // random solid color
      isFinished = animations::doubleSideFillUp(randomColor, 1000, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
    break;

    case 2:
      isFinished = switchMode ? animations::colorWipeUp(complColor, 1000, isFinished, strip) : animations::colorWipeDown(complColor, 1000, isFinished, strip);
      if (isFinished)
      {
        switchMode = !switchMode;
        complColor.update();  // update color
      }
    break;

    case 3:
      sound::vu_meter(gradientColor, strip);
    break;

    case 4:
      isFinished = switchMode ? animations::fadeOut(300, isFinished, strip) : animations::fadeIn(rainbowColor, 1000, isFinished, strip);
      if (isFinished) switchMode = !switchMode;
    break;

    case 5:
      animations::police(400, false, strip);
    break;

    case 6:
      isFinished = animations::fadeIn(rainbowPulse, 5000, isFinished, strip);
      if (isFinished)
      {
        rainbowPulse.update();
      }
    break;

    case 7:
      // ping pong a color for infinity
      isFinished = animations::dotPingPong(complColor, 1000.0, isFinished, strip);
      if (isFinished) complColor.update();  // update color
    break;

    case 8:
    if (paletteColor.update()) animations::fill(paletteColor, strip);
    break;

    case 9:
    // wipe a color pulse around the tube at each beat
    if (sound::pulse_beat_wipe(complColor, strip)) complColor.update();
    break;

    case 10:
    animations::fill(rdColor, strip);
    break;

    case 11:
    isFinished = animations::fadeIn(paletteColor, timing, isFinished, strip);
    if (isFinished)
    {
      paletteColor.update();  // update color
      timing = 50 + rand()/(float)RAND_MAX * 50;
    }
    break;

    default:
    strip.clear();
    strip.show();  //  Update strip to match
    break;
  }
}

bool isAugmentBrightness = true;
// call when the button is finally release
void buttonClickedCallback(uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  isAugmentBrightness = true;
  switch(consecutiveButtonCheck)
  {
    case 1: // 1 press
      if (isActivated)
        incrementColorWheel();
      else
        // activate with one click
        isActivated = true;
      break;

    case 2: // 1 presses
      if (isActivated)
        decrement_color_wheel();
      break;
    
    default:
    // error
      break;
  }
}


#define BRIGHTNESS_RAMP_DURATION_MS 3000

void buttonHoldCallback(uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration)
{
  // no click event
  if (consecutiveButtonCheck == 0)
  {
    return;
  }
  const bool isEndOfHoldEvent = buttonHoldDuration <= 1;

  switch(consecutiveButtonCheck)
  {
    case 1:
    // deactivate on 1 second
      if (isActivated and buttonHoldDuration > 1000)
      {
        isActivated = false;
        isFinished = true;
        switchMode = false;
      }
      break;

    case 2:
      // augment luminositity
      if (isAugmentBrightness)
      {
        if (!isEndOfHoldEvent)
        {
          BRIGHTNESS = max(BRIGHTNESS, map(min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS), HOLD_BUTTON_MIN_MS, BRIGHTNESS_RAMP_DURATION_MS, MIN_BRIGHTNESS, MAX_BRIGHTNESS) );
          strip.setBrightness(BRIGHTNESS);
        }
        else
          // switch brigtness
          isAugmentBrightness = false;
      }
      else
      {
         // lower luminositity
        if (!isEndOfHoldEvent)
        {
          BRIGHTNESS = min(BRIGHTNESS, map(min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS), HOLD_BUTTON_MIN_MS, BRIGHTNESS_RAMP_DURATION_MS, MAX_BRIGHTNESS, MIN_BRIGHTNESS) );
          strip.setBrightness(BRIGHTNESS);
        }
        else
          // switch brigtness
          isAugmentBrightness = true;
      }
      break;

    case 3:
      break;
    
    default:
    // error
      break;
  }
}