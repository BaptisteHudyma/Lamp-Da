#include "behavior.h"

#include "physical/MicroPhone.h"
#include "physical/button.h"
#include "physical/fileSystem.h"
#include "physical/battery.h"

#include "colors/animations.h"
#include "colors/palettes.h"
#include "colors/wipes.h"

#include "utils/colorspace.h"
#include "utils/constants.h"
#include "utils/text.h"
#include "utils/utils.h"

#include "ext/math8.h"
#include "ext/noise.h"

#include "alerts.h"

#include <cstdint>


// extern declarations
LedStrip strip(LED_PIN, NEO_RGB);

uint8_t BRIGHTNESS = 50;
const char* brightnessKey = "brightness";

// constantes
static constexpr uint8_t MIN_BRIGHTNESS = 2;
// max brightness depends on max power consumption
static const uint8_t MAX_BRIGHTNESS = min(1.0, maxPowerConsumption_A / maxStripConsumption_A) * 255;


bool modeChange = true;     // signal a color mode change
bool categoryChange = true; // signal a color category change

uint8_t colorMode = 0;    // color mode: main wheel of the color mode
const char* colorModeKey = "colorMode";

uint8_t colorState = 0;   // color state: subwheel of the current color mode
const char* colorStateKey = "colorState";

uint8_t colorCodeIndex = 0; // color code index, used for color indexion
uint8_t lastColorCodeIndex = colorCodeIndex;

uint8_t colorCodeIndexForWarmLight = 0; // color code index, used for color indexion of warm to white
const char* colorCodeIndexForWarmLightKey = "warmLight";

uint8_t colorCodeIndexForColoredLight = 0; // color code index, used for color indexion of RGB
const char* colorCodeIndexForColoredLightKey = "colorLight";

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
}

void increment_color_state()
{
  colorState += 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
}

void decrement_color_state()
{
  colorState -= 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
}

void read_parameters()
{
  fileSystem::load_initial_values();

  uint32_t brightness = 0;
  if (fileSystem::get_value(std::string(brightnessKey), brightness))
  {
    // update it directly (bad design)
    BRIGHTNESS = brightness;
    strip.setBrightness(BRIGHTNESS);
  }

  uint32_t mode = 0;
  if (fileSystem::get_value(std::string(colorModeKey), mode))
  {
    colorMode = mode;
  }

  uint32_t state = 0;
  if (fileSystem::get_value(std::string(colorStateKey), state))
  {
    colorState = state;
  }

  uint32_t warmLight = 0;
  if (fileSystem::get_value(std::string(colorCodeIndexForWarmLightKey), warmLight))
  {
    colorCodeIndexForWarmLight = warmLight;
  }

  uint32_t coloredLight = 0;
  if (fileSystem::get_value(std::string(colorCodeIndexForColoredLightKey), coloredLight))
  {
    colorCodeIndexForColoredLight = coloredLight;
  }
}

void write_parameters()
{
    fileSystem::clear();
    fileSystem::set_value(std::string(brightnessKey), BRIGHTNESS);
    fileSystem::set_value(std::string(colorModeKey), colorMode);
    fileSystem::set_value(std::string(colorStateKey), colorState);
    fileSystem::set_value(std::string(colorCodeIndexForWarmLightKey), colorCodeIndexForWarmLight);
    fileSystem::set_value(std::string(colorCodeIndexForColoredLightKey), colorCodeIndexForColoredLight);

    fileSystem::write_state();
}

void shutdown()
{
  // remove all colors from strip
  strip.clear();
  strip.show();  //  Update strip to match

  // deactivate strip power
  digitalWrite(LED_POWER_PIN, LOW);

  // disable bluetooth and microphone
  sound::disable_microphone();
#ifdef USE_BLUETOOTH
  bluetooth::disable_bluetooth();
#endif

  // save the current config to a file (akes some time so call it when the lamp appear to be shutdown already)
  write_parameters();

  // deactivate indicators
  digitalWrite(LED_BUILTIN, HIGH);
  set_button_color(utils::ColorSpace::RGB(0,0,0));

  // setup your wake-up pins.
  pinMode(BUTTON_PIN,  INPUT_PULLUP_SENSE);// this pin is pulled up and wakes up the board when externally connected to ground.
  
  // write delay
  delay(100);
  
  // power down nrf52.
  // If no sense pins are setup (or other hardware interrupts), the nrf52 will not wake up.
  sd_power_system_off();                    // this function puts the whole nRF52 to deep sleep.
  // on wake up, it'll start back from the setup phase
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
  static auto lastColorStep = colorCodeIndex;

  constexpr uint8_t maxGradientColorState = 1;
  switch(clamp_state_values(colorState, maxGradientColorState))
  {
    case 0: // kelvin mode
      static auto paletteHeatColor = GeneratePaletteIndexed(PaletteBlackBodyColors);
      if (categoryChange)
      {
        lastColorStep = 1;
        colorCodeIndex = colorCodeIndexForWarmLight;
        lastColorCodeIndex = colorCodeIndexForWarmLight;
        paletteHeatColor.reset();
      }

      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        colorCodeIndexForWarmLight = colorCodeIndex;
        paletteHeatColor.update(colorCodeIndex);
      }
      animations::fill(paletteHeatColor, strip);
      break;

    case 1: // rainbow mode
      static auto rainbowIndex = GenerateRainbowIndex(UINT8_MAX);     // pulse around a rainbow, with a certain color division
      if (categoryChange)
      {
        lastColorStep = 1;
        colorCodeIndex = colorCodeIndexForColoredLight;
        lastColorCodeIndex = colorCodeIndexForColoredLight;
        rainbowIndex.reset();
      }
      
      if(colorCodeIndex != lastColorStep)
      {
        lastColorStep = colorCodeIndex;
        colorCodeIndexForColoredLight = colorCodeIndex;
        rainbowIndex.update(colorCodeIndex);
      }
      animations::fill(rainbowIndex, strip);
      break;

    default:  // error
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_STATE);
      colorState = 0;
      colorCodeIndex = 0;
      strip.clear();
    break;
  }

  // reset category change
  categoryChange = false;
}

void calm_mode_update()
{
  constexpr uint8_t maxCalmColorState = 9;
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

      isFinished = animations::fade_in(palettePartyColor, 100, isFinished, strip);
      if (isFinished)
      {
        currentIndex++;
        palettePartyColor.update(currentIndex);
      }
      break;

    case 2:
      animations::random_noise(PaletteLavaColors, strip, categoryChange, true, 3);
    break;

    case 3:
      animations::random_noise(PaletteForestColors, strip, categoryChange, true, 3);
    break;

    case 4:
      animations::random_noise(PaletteOceanColors, strip, categoryChange, true, 3);
    break;

    case 5:
      animations::random_noise(PalettePartyColors, strip, categoryChange, true, 3);
    break;
    
    case 6:
      // polar light
      animations::mode_2DPolarLights(255, 128, PaletteAuroraColors, categoryChange, strip);
      break;
    
    case 7:
      animations::fire(60, 60, 255, PaletteHeatColors, strip);
    break;

    case 8:
      animations::mode_2Ddistortionwaves(128, 128, strip);
    break;

    case 9:
      animations::mode_2DDrift(255, 64, PaletteRainbowColors, strip);
    break;

    default:  // error
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_STATE);
      colorState = 0;
      strip.clear();
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

      isFinished = switchMode ? animations::color_wipe_up(complementaryColor, 500, isFinished, strip) : animations::color_wipe_down(complementaryColor, 500, isFinished, strip);
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

      isFinished = animations::double_side_fill(randomColor, 500, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
    break;

    case 2:
      static GenerateComplementaryColor complementaryPingPongColor = GenerateComplementaryColor(0.3);
      if (categoryChange) complementaryPingPongColor.reset();

      // ping pong a color for infinity
      isFinished = animations::dot_ping_pong(complementaryPingPongColor, 1000.0, isFinished, strip);
      if (isFinished) complementaryPingPongColor.update();  // update color
    break;

    default:  // error
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_STATE);
      colorState = 0;
      strip.clear();
    break;
  }

  // reset category change
  categoryChange = false;
}

void sound_mode_update()
{
  constexpr uint8_t maxSoundState = 2;
  switch(clamp_state_values(colorState, maxSoundState))
  {
    case 0: // vue meter
      static GenerateGradientColor redToGreenGradient = GenerateGradientColor(LedStrip::Color(0, 255, 0), LedStrip::Color(255, 0, 0)); // gradient from red to green
      if (categoryChange) redToGreenGradient.reset();

      sound::vu_meter(redToGreenGradient, strip);
    break;
 
    case 1: // pulse soud
      sound::fftDisplay(128, 128, PaletteHeatColors, categoryChange, strip, 255);

    break;

    case 2:
      sound::mode_2DWaverly(128, 64, PalettePartyColors, strip);
    break;

    default:  // error
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_STATE);
      colorState = 0;
      strip.clear();
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
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_STATE);
      colorState = 0;
      strip.clear();
    break;
  }

  // reset category change
  categoryChange = false;
}

// Display a color on the button
void display_battery_level()
{
  static constexpr uint32_t refreshRate_ms = 1000;
  static uint32_t lastCall = 0;

  const uint32_t newCall = millis();
  if(newCall - lastCall > refreshRate_ms or lastCall == 0)
  {
    lastCall = newCall;
    const uint8_t percent = get_battery_level(false);

    // 5% battery is critical
    if(percent <= 5)
    {
      AlertManager.raise_alert(Alerts::BATTERY_CRITICAL);
    }
    else {
      AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);
      
      // 10% battery is low, start alerting
      if(percent <= 10)
      {
        AlertManager.raise_alert(Alerts::BATTERY_LOW);
      }
      else {
        AlertManager.clear_alert(Alerts::BATTERY_LOW);
      }
    }
  }
}

void color_mode_update()
{
  // start by displaying the battery level
  display_battery_level();

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
      AlertManager.raise_alert(Alerts::UNKNOWN_COLOR_MODE);
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

  switch(consecutiveButtonCheck)
  {
    case 1: // 1 click: shutdown
      shutdown();
      break;

    case 2: // 2 clicks: increment color state
      increment_color_state();
      break;

    case 3: // 2 clicks: decrement color state
      decrement_color_state();
      break;
    
    case 4: // 4 clicks: increment color mode
      increment_color_mode();
      break;

    case 5: // 5 clicks: decrement color mode
      decrement_color_mode();
      break;

    default:
    // unhandled
      break;
  }
}


#define BRIGHTNESS_RAMP_DURATION_MS 4000

void button_hold_callback(uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration)
{
  // no click event
  if (consecutiveButtonCheck == 0)
    return;

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
        constexpr uint32_t colorStepDuration_ms = 6000;
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
        constexpr uint32_t colorStepDuration_ms = 6000;
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
    // unhandled
      break;
  }
}

void handle_alerts()
{
  const uint32_t current = AlertManager.current();
  
  static uint32_t criticalbatteryRaisedTime = 0; 

  if(current == Alerts::NONE)
  {
    criticalbatteryRaisedTime = 0;

    // display battery level
    // red to green
    // force green to be kind of low because red is not as powerfull
    set_button_color(utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RGB(255, 0, 0).get_rgb().color, utils::ColorSpace::RGB(0,30,0).get_rgb().color, batteryLevel / 100.0)));
  }
  else {
    if( (current & Alerts::BATTERY_READINGS_INCOHERENT) != 0x00)
    {
      // incohrent battery readings
      button_blink(100, 100, utils::ColorSpace::GREEN);
    }
    else if( (current & Alerts::BATTERY_CRITICAL) != 0x00)
    {
      // critical battery alert: shutdown after 2 seconds
      if(criticalbatteryRaisedTime == 0)
        criticalbatteryRaisedTime = millis();
      else if(millis() - criticalbatteryRaisedTime > 2000)
      {
        // shutdown when battery is critical
        shutdown();
      }
      // blink if no shutdown
      button_blink(100, 100, utils::ColorSpace::RED);
    }
    else if( (current & Alerts::BATTERY_LOW) != 0x00)
    {
      criticalbatteryRaisedTime = 0;
      // fast blink red
      button_blink(300, 300, utils::ColorSpace::RED);
    }
    else if( (current & Alerts::LONG_LOOP_UPDATE) != 0x00)
    {
      // fast blink red
      button_blink(400, 400, utils::ColorSpace::FUSHIA);
    }
    else
    {
      // unhandled case
      button_blink(300, 300, utils::ColorSpace::ORANGE);
    }
  }
}