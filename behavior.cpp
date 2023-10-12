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


bool modeChange = true;     // signal a color mode change
bool categoryChange = true; // signal a color category change

uint8_t colorMode = 0;    // color mode: main wheel of the color mode
uint8_t colorState = 0;   // color state: subwheel of the current color mode

uint8_t colorCodeIndex = 0; // color code index, used for color indexion
uint8_t lastColorCodeIndex = colorCodeIndex;

bool displayBattery = false;
uint32_t displayBatteryTime = 0;

bool isFinished = true;
bool switchMode = true;

void reset_globals()
{
  isFinished = true;
  switchMode = false;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;
}

void increment_color_mode()
{
  colorMode += 1;
  modeChange = true;

  // reset color state
  colorState = 0;
  categoryChange = true;
  
  reset_globals();

  strip.clear();
  strip.show();  //  Update strip to match
}

void decrement_color_mode()
{
  colorMode -= 1;
  modeChange = true;

  // reset color state
  colorState = 0;
  categoryChange = true;
  
  reset_globals();

  strip.clear();
  strip.show();  //  Update strip to match
}

void increment_color_state()
{
  colorState += 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
  strip.show();  //  Update strip to match
}

void decrement_color_state()
{
  colorState -= 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
  strip.show();  //  Update strip to match
}



uint8_t clamp_state_values(uint8_t& state, const uint8_t maxValue)
{
  // incrmeent one too much, loop around
  if (state == maxValue + 1)
    state = 0;

  // default return value
  else if (state <= maxValue)
    return state;

  // got below 0, set to max value
  else if (state > maxValue)
    state = maxValue;

  return state;
}



void gradient_mode_update()
{
  constexpr uint8_t maxGradientColorState = 1;
  switch(clamp_state_values(colorState, maxGradientColorState))
  {
    case 0: // kelvin mode
      static auto lastColorStep = colorCodeIndex;
      static auto paletteHeatColor = GeneratePaletteIndexed(PaletteBlackBodyColors);
      if (categoryChange) paletteHeatColor.reset();

      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        paletteHeatColor.update(colorCodeIndex);
      }
      animations::fill(paletteHeatColor, strip);
      break;

    case 1: // rainbow mode
      static auto rainbowIndex = GenerateRainbowIndex(UINT8_MAX);     // pulse around a rainbow, with a certain color division
      if (categoryChange) rainbowIndex.reset();
      
      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        rainbowIndex.update(colorCodeIndex);
      }
      animations::fill(rainbowIndex, strip);
      break;

    default:  // error
      colorState = 0;
      strip.clear();
      strip.show(); // clear strip
    break;
  }

  // reset category change
  categoryChange = false;
}

void calm_mode_update()
{
  constexpr uint8_t maxCalmColorState = 3;
  switch(clamp_state_values(colorState, maxCalmColorState))
  {
    case 0: // rainbow swirl animation
      // display a color animation
      static GenerateRainbowSwirl rainbowSwirl = GenerateRainbowSwirl(5000);  // swirl animation (5 seconds)
      if (categoryChange) rainbowSwirl.reset();

      animations::fill(rainbowSwirl, strip);
      rainbowSwirl.update();  // update 
    break;

    case 1: // party wheel
      static auto lastColorStep = colorCodeIndex;
      static auto palettePartyColor = GeneratePaletteIndexed(PalettePartyColors);
      static uint8_t currentIndex = 0;
      if (categoryChange)
      {
        currentIndex = 0;
        palettePartyColor.reset();
      }

      isFinished = animations::fadeIn(palettePartyColor, 100, isFinished, strip);
      if (isFinished)
      {
        currentIndex++;
        palettePartyColor.update(currentIndex);
      }
      break;

    case 2:
      animations::random_noise(PalettePartyColors, strip, true, 20);
    break;

    case 3: // pastel wheel
      static GenerateRainbowPulse rainbowPulse = GenerateRainbowPulse(25);     // pulse around a rainbow, with a certain color division
      if (categoryChange) rainbowPulse.reset();

      isFinished = animations::fadeIn(rainbowPulse, 300, isFinished, strip);
      if (isFinished)
      {
        rainbowPulse.update();
      }
    break;

    default:  // error
      colorState = 0;
      strip.clear();
      strip.show(); // clear strip
    break;
  }

  // reset category change
  categoryChange = false;
}

void party_mode_update()
{
  constexpr uint8_t maxPartyState = 2;
  switch(clamp_state_values(colorState, maxPartyState))
  {
    case 0:
      static GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
      if (categoryChange) complementaryColor.reset();

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
      if (categoryChange) randomColor.reset();

      isFinished = animations::doubleSideFillUp(randomColor, 500, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
    break;

    case 2:
      static GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.3);
      if (categoryChange) complementaryPingPongColor.reset();

      // ping pong a color for infinity
      isFinished = animations::dotPingPong(complementaryPingPongColor, 500.0, isFinished, strip);
      if (isFinished) complementaryPingPongColor.update();  // update color
    break;

    default:  // error
    colorState = 0;
      strip.clear();
      strip.show(); // clear strip
    break;
  }

  // reset category change
  categoryChange = false;
}

void sound_mode_update()
{
  constexpr uint8_t maxSoundState = 1;
  switch(clamp_state_values(colorState, maxSoundState))
  {
    case 0: // vue meter
      static GenerateGradientColor redToGreenGradient = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
      if (categoryChange) redToGreenGradient.reset();

      sound::vu_meter(redToGreenGradient, strip);
    break;

    case 1: // pulse soud
      // wipe a color pulse around the tube at each beat
      static GenerateComplementaryColor complementaryColor = GenerateComplementaryColor(0.3);
      if (categoryChange) redToGreenGradient.reset();

      if (sound::pulse_beat_wipe(complementaryColor, strip))
        complementaryColor.update();
    break;

    default:  // error
      colorState = 0;
      strip.clear();
      strip.show(); // clear strip
    break;
  }

  // reset category change
  categoryChange = false;
}

void gyro_mode_update()
{
  constexpr uint8_t maxGyroState = 0;
  switch(clamp_state_values(colorState, maxGyroState))
  {
    case 0:
      animations::police(500, false, strip);
    break;

    default:  // error
      colorState = 0;
      strip.clear();
      strip.show(); // clear strip
    break;
  }

  // reset category change
  categoryChange = false;
}

void display_battery_level()
{
  static GenerateGradientColor redToGreenGradient = GenerateGradientColor(Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0)); // gradient from red to green
  if (categoryChange) redToGreenGradient.reset();

  strip.clear();
  animations::fill(redToGreenGradient, strip, get_battery_level() / 100.0);

  // animation last for 5 seconds max
  if (millis() - displayBatteryTime > 5000.0)
  {
    strip.clear();
    strip.show();
    displayBattery = false;
  }
}

void color_mode_update()
{

  if(displayBattery)
  {
    // display battery voltage
    display_battery_level();
    return;
  }

  constexpr uint8_t maxColorMode = 4;
  switch(clamp_state_values(colorMode, maxColorMode))
  {
    case 0: // gradient mode
      gradient_mode_update();
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
      colorMode = 0;
    break;
  }

  // reset mode change
  modeChange = false;
}


// call when the button is finally release
void button_clicked_callback(uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;

  displayBattery = false;

  switch(consecutiveButtonCheck)
  {
    case 1: // 1 click: toggle lamp
      isActivated = !isActivated;
      reset_globals();
      break;

    case 2: // 2 clicks: increment color state
      if (isActivated)
        increment_color_state();
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
  // ignore buttons events when lamp is off
  if (!isActivated)
    return;

  displayBattery = false;

  const bool isEndOfHoldEvent = buttonHoldDuration <= 1;
  buttonHoldDuration -= HOLD_BUTTON_MIN_MS;

  // hold the current level of brightness out of the animation
  static uint8_t currentBrightness = BRIGHTNESS;

  switch(consecutiveButtonCheck)
  {
    case 1: // just hold the click
      if (!isEndOfHoldEvent)
      {
        const float percentOfTimeToGoUp = float(MAX_BRIGHTNESS - currentBrightness) / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

        const auto newBrightness = map(
          min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp), 0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
          currentBrightness, MAX_BRIGHTNESS
        );
        if (BRIGHTNESS != newBrightness)
        {
          BRIGHTNESS = newBrightness;
          strip.setBrightness(BRIGHTNESS);
        }
      }
      else
      {
        // switch brigtness
        currentBrightness = BRIGHTNESS;
      }
      break;

    case 2: // 2 click and hold
        // lower luminositity
      if (!isEndOfHoldEvent)
      {
        const double percentOfTimeToGoDown = float(currentBrightness - MIN_BRIGHTNESS) / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

        const auto newBrightness = map(
          min(buttonHoldDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown), 0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown,
          currentBrightness, MIN_BRIGHTNESS
        );
        if (BRIGHTNESS != newBrightness)
        {
          BRIGHTNESS = newBrightness;
          strip.setBrightness(BRIGHTNESS);
        }
      }
      else
      {
        // switch brigtness
        currentBrightness = BRIGHTNESS;
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