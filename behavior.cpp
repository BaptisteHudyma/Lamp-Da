#include "behavior.h"

#include "MicroPhone.h"

#include "button.h"
#include "animations.h"
#include "wipes.h"
#include "utils.h"
#include <cstdint>


// extern declarations
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB);

uint8_t BRIGHTNESS = 50;

bool isActivated = false;


// constantes
constexpr uint8_t MIN_BRIGHTNESS = 15;
constexpr uint8_t MAX_BRIGHTNESS = 255;

uint8_t colorMode = 0;    // color mode: main wheel of the color mode
uint8_t colorState = 0;   // color state: subwheel of the current color mode

uint8_t colorCodeIndex = 0; // color code index, used for color indexion
uint8_t lastColorCodeIndex = colorCodeIndex;

bool displayBattery = false;
uint32_t displayBatteryTime = 0;

bool isFinished = true;
bool switchMode = true;

void increment_color_mode()
{
  colorMode += 1;

  colorState = 0;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;
  isFinished = true;
  switchMode = false;

  strip.clear();
  strip.show();  //  Update strip to match
}

void decrement_color_mode()
{
  colorMode -= 1;

  colorState = 0;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;
  isFinished = true;
  switchMode = false;

  strip.clear();
  strip.show();  //  Update strip to match
}

void increment_color_state()
{
  colorState += 1;

  isFinished = true;
  switchMode = false;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;

  strip.clear();
  strip.show();  //  Update strip to match
}

void decrement_color_state()
{
  colorState -= 1;

  isFinished = true;
  switchMode = false;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;

  strip.clear();
  strip.show();  //  Update strip to match
}







void kelvin_mode_update()
{
  constexpr uint8_t maxKelvinColorState = 2;
  switch(colorState % maxKelvinColorState)
  {
    case 0: // kelvin mode
      static auto lastColorStep = colorCodeIndex;
      static auto paletteHeatColor = GeneratePaletteIndexed(PaletteHeatColors);
      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        paletteHeatColor.update(colorCodeIndex);
      }
      animations::fill(paletteHeatColor, strip);
      break;

    case 1: // rainbow mode
      static auto rainbowIndex = GenerateRainbowIndex(UINT8_MAX);     // pulse around a rainbow, with a certain color division
      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        rainbowIndex.update(colorCodeIndex);
      }
      animations::fill(rainbowIndex, strip);
      break;

    default:  // error
      strip.clear();
      strip.show(); // clear strip
    break;
  }
}

void calm_mode_update()
{
  constexpr uint8_t maxCalmColorState = 3;
  switch(colorState % maxCalmColorState)
  {
    case 0: // rainbow swirl animation
      // display a color animation
      static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);  // swirl animation (5 seconds)
      animations::fill(rainbowSwirl, strip);
      rainbowSwirl.update();  // update 
    break;

    case 1:
      static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(10);     // pulse around a rainbow, with a certain color division
      isFinished = animations::fadeIn(rainbowPulse, 5000, isFinished, strip);
      if (isFinished)
      {
        rainbowPulse.update();
      }
    break;

    case 2: // slow color change
      static GenerateRainbowColor rainbowColor = GenerateRainbowColor();      // will output a rainbow from start to bottom of the display
      isFinished = switchMode ? animations::fadeOut(300, isFinished, strip) : animations::fadeIn(rainbowColor, 1000, isFinished, strip);
      if (isFinished) switchMode = !switchMode;
    break;

    default:  // error
      strip.clear();
      strip.show(); // clear strip
    break;
  }
}

void party_mode_update()
{
  constexpr uint8_t maxPartyState = 3;
  switch(colorState % maxPartyState)
  {
    case 0:
      static GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
      isFinished = switchMode ? animations::colorWipeUp(complementaryColor, 500, isFinished, strip) : animations::colorWipeDown(complementaryColor, 500, isFinished, strip);
      if (isFinished)
      {
        switchMode = !switchMode;
        complementaryColor.update();  // update color
      }
    break;

    case 1:
      // random solid color
      static GenerateRandomColor randomColor = GenerateRandomColor();
      isFinished = animations::doubleSideFillUp(randomColor, 500, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
    break;

    case 2:
      static GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.3);
      // ping pong a color for infinity
      isFinished = animations::dotPingPong(complementaryPingPongColor, 500.0, isFinished, strip);
      if (isFinished) complementaryPingPongColor.update();  // update color
    break;

    default:  // error
      strip.clear();
      strip.show(); // clear strip
    break;
  }
}

void sound_mode_update()
{
  constexpr uint8_t maxSoundState = 2;
  switch(colorState % maxSoundState)
  {
    case 0: // vue meter
      static GenerateGradientColor redToGreenGradient = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
      sound::vu_meter(redToGreenGradient, strip);
    break;

    case 1: // pulse soud
      // wipe a color pulse around the tube at each beat
      static GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
      if (sound::pulse_beat_wipe(complementaryColor, strip))
        complementaryColor.update();
    break;

    default:  // error
      strip.clear();
      strip.show(); // clear strip
    break;
  }
}

void gyro_mode_update()
{
  constexpr uint8_t maxGyroState = 1;
  switch(colorState % maxGyroState)
  {
    case 0:
      animations::police(400, false, strip);
    break;

    default:  // error
      strip.clear();
      strip.show(); // clear strip
    break;
  }
}

void color_mode_update()
{
  constexpr uint8_t maxColorMode = 5;

  if(displayBattery)
  {
    static GenerateGradientColor redToGreenGradient = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
    strip.clear();
    animations::fill(redToGreenGradient, strip, get_battery_level() / 100.0);

    // animation last for 5 seconds max
    if (millis() - displayBatteryTime > 5000.0)
    {
      strip.clear();
      strip.show();
      displayBattery = false;
    }

    // display battery voltage
    return;
  }

  switch(colorMode % maxColorMode)
  {
    case 0: // kelvin mode
      kelvin_mode_update();
    break;

    case 1: // calm mode
      calm_mode_update();
    break;

    case 2:
      party_mode_update();
    break;

    case 3: // sound mode
      sound_mode_update();
    break;

    case 4: // gyrophare
      gyro_mode_update();
    break;

    default:
    break;
  }
}


bool isAugmentBrightness = true;
// call when the button is finally release
void button_clicked_callback(uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  displayBattery = false;

  isAugmentBrightness = true;
  switch(consecutiveButtonCheck)
  {
    case 1: // 1 click: activate lamp, or increment mode
      if (isActivated)
        increment_color_state();
      else
        // activate with one click
        isActivated = true;
      break;

    case 2: // 2 clicks: decrement color state
      if (isActivated)
        decrement_color_state();
      break;
    
    case 3: // 3 clicks: increment color mode
      if (isActivated)
        increment_color_mode();
      break;

    case 4: // 4 clicks: decrement color mode
      if (isActivated)
        decrement_color_mode();
      break;

    case 5:
      // TODO: bluetooth
      displayBattery = true;
      displayBatteryTime = millis();
      break;

    default:
    // error
      break;
  }
}


#define BRIGHTNESS_RAMP_DURATION_MS 4000

void button_hold_callback(uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration)
{
  // no click event
  if (consecutiveButtonCheck == 0)
    return;

  displayBattery = false;

  const bool isEndOfHoldEvent = buttonHoldDuration <= 1;
  buttonHoldDuration -= HOLD_BUTTON_MIN_MS;

  // hold the current level of brightness out of the animation
  static uint8_t currentBrightness = BRIGHTNESS;

  switch(consecutiveButtonCheck)
  {
    case 1: // just hold the click
    // deactivate on 1 second
      if (isActivated and buttonHoldDuration > 1000)
      {
        isActivated = false;
        isFinished = true;
        switchMode = false;
      }
      break;

    case 2: // 2 click and hold
      // augment luminositity
      if (isAugmentBrightness)
      {
        if (!isEndOfHoldEvent)
        {
          const auto newBrightness = map(min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS), 0, BRIGHTNESS_RAMP_DURATION_MS, currentBrightness, MAX_BRIGHTNESS);
          if (BRIGHTNESS != newBrightness)
          {
            BRIGHTNESS = newBrightness;
            strip.setBrightness(BRIGHTNESS);
          }
        }
        else
        {
          // switch brigtness
          isAugmentBrightness = false;
          currentBrightness = BRIGHTNESS;
        }
      }
      else
      {
         // lower luminositity
        if (!isEndOfHoldEvent)
        {
          const auto newBrightness = map(min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS), 0, BRIGHTNESS_RAMP_DURATION_MS, currentBrightness, MIN_BRIGHTNESS);
          if (BRIGHTNESS != newBrightness)
          {
            BRIGHTNESS = newBrightness;
            strip.setBrightness(BRIGHTNESS);
          }
        }
        else
        {
          // switch brigtness
          isAugmentBrightness = true;
          currentBrightness = BRIGHTNESS;
        }
      }
      break;

    case 3: // 3 clicks and hold
      if (!isEndOfHoldEvent)
      {
        constexpr uint32_t colorStepDuration_ms = 10000;
        const uint32_t timeShift = (colorStepDuration_ms * lastColorCodeIndex)/255;
        const uint32_t colorStep  = (buttonHoldDuration + timeShift) % colorStepDuration_ms;
        colorCodeIndex = map(colorStep, 0, colorStepDuration_ms, 0, UINT8_MAX);
      }
      else {
        lastColorCodeIndex = colorCodeIndex;
      }
      break;

    case 4: // 4 clicks and hold
      if (!isEndOfHoldEvent)
      {
        constexpr uint32_t colorStepDuration_ms = 10000;
        const uint32_t timeShift = (colorStepDuration_ms * lastColorCodeIndex)/255;
        if (buttonHoldDuration < timeShift)
        {
          buttonHoldDuration = colorStepDuration_ms - (timeShift - buttonHoldDuration);
        }
        else {
          buttonHoldDuration -= timeShift;
        }
        const uint32_t colorStep  = buttonHoldDuration % colorStepDuration_ms;
        colorCodeIndex = map(colorStep, 0, colorStepDuration_ms, UINT8_MAX, 0);
      }
      else {
        lastColorCodeIndex = colorCodeIndex;
      }
      break;
    
    default:
    // error
      break;
  }
}