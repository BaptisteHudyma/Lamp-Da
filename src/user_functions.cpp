#include "user_functions.h"

#include "system/behavior.h"
#include "system/charger/charger.h"
#include "system/colors/animations.h"
#include "system/colors/colors.h"
#include "system/colors/palettes.h"
#include "system/colors/wipes.h"
#include "system/physical/IMU.h"
#include "system/physical/MicroPhone.h"
#include "system/physical/fileSystem.h"

namespace user {

LedStrip strip(AD0);

constexpr uint32_t LED_POWER_PIN = AD1;

bool modeChange = true;      // signal a color mode change
bool categoryChange = true;  // signal a color category change

uint8_t colorMode = 0;  // color mode: main wheel of the color mode
const char* colorModeKey = "colorMode";

uint8_t colorState = 0;  // color state: subwheel of the current color mode
const char* colorStateKey = "colorState";

uint8_t colorCodeIndex = 0;  // color code index, used for color indexion
uint8_t lastColorCodeIndex = colorCodeIndex;

uint8_t colorCodeIndexForWarmLight =
    0;  // color code index, used for color indexion of warm to white
const char* colorCodeIndexForWarmLightKey = "warmLight";

uint8_t colorCodeIndexForColoredLight =
    0;  // color code index, used for color indexion of RGB
const char* colorCodeIndexForColoredLightKey = "colorLight";

bool isFinished = true;
bool switchMode = true;

void reset_globals() {
  isFinished = true;
  switchMode = false;
  colorCodeIndex = 0;
  lastColorCodeIndex = 0;
}

void increment_color_mode() {
  colorMode += 1;
  modeChange = true;

  // reset color state
  colorState = 0;
  categoryChange = true;

  reset_globals();

  strip.clear();
}

void decrement_color_mode() {
  colorMode -= 1;
  modeChange = true;

  // reset color state
  colorState = 0;
  categoryChange = true;

  reset_globals();

  strip.clear();
}

void increment_color_state() {
  colorState += 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
}

void decrement_color_state() {
  colorState -= 1;

  // signal a change of category
  categoryChange = true;

  reset_globals();

  strip.clear();
}

uint8_t clamp_state_values(uint8_t& state, const uint8_t maxValue) {
  // incrmeent one too much, loop around
  if (state == maxValue + 1) state = 0;

  // default return value
  else if (state <= maxValue)
    return state;

  // got below 0, set to max value
  else if (state > maxValue)
    state = maxValue;

  return state;
}

void gradient_mode_update() {
  static auto lastColorStep = colorCodeIndex;

  constexpr uint8_t maxGradientColorState = 1;
  switch (clamp_state_values(colorState, maxGradientColorState)) {
    case 0:  // kelvin mode
      static auto paletteHeatColor =
          GeneratePaletteIndexed(PaletteBlackBodyColors);
      if (categoryChange) {
        lastColorStep = 1;
        colorCodeIndex = colorCodeIndexForWarmLight;
        lastColorCodeIndex = colorCodeIndexForWarmLight;
        paletteHeatColor.reset();
      }

      if (colorCodeIndex != lastColorStep) {
        lastColorStep = colorCodeIndex;
        colorCodeIndexForWarmLight = colorCodeIndex;
        paletteHeatColor.update(colorCodeIndex);
      }
      animations::fill(paletteHeatColor, strip);
      break;

    case 1:  // rainbow mode
      static auto rainbowIndex = GenerateRainbowIndex(
          UINT8_MAX);  // pulse around a rainbow, with a certain color division
      if (categoryChange) {
        lastColorStep = 1;
        colorCodeIndex = colorCodeIndexForColoredLight;
        lastColorCodeIndex = colorCodeIndexForColoredLight;
        rainbowIndex.reset();
      }

      if (colorCodeIndex != lastColorStep) {
        lastColorStep = colorCodeIndex;
        colorCodeIndexForColoredLight = colorCodeIndex;
        rainbowIndex.update(colorCodeIndex);
      }
      animations::fill(rainbowIndex, strip);
      break;

    default:  // error
      colorState = 0;
      colorCodeIndex = 0;
      strip.clear();
      break;
  }

  // reset category change
  categoryChange = false;
}

void calm_mode_update() {
  constexpr uint8_t maxCalmColorState = 9;
  switch (clamp_state_values(colorState, maxCalmColorState)) {
    case 0:  // rainbow swirl animation
    {        // display a color animation
      static GenerateRainbowSwirl rainbowSwirl =
          GenerateRainbowSwirl(5000);  // swirl animation (5 seconds)
      if (categoryChange) rainbowSwirl.reset();

      animations::fill(rainbowSwirl, strip);

      rainbowSwirl.update();
      break;
    }
    case 1:  // party wheel
    {
      static auto palettePartyColor =
          GeneratePaletteIndexed(PalettePartyColors);
      static uint8_t currentIndex = 0;
      if (categoryChange) {
        currentIndex = 0;
        palettePartyColor.reset();
      }

      isFinished =
          animations::fade_in(palettePartyColor, 100, isFinished, strip);
      if (isFinished) {
        currentIndex++;
        palettePartyColor.update(currentIndex);
      }
      break;
    }
    case 2: {
      animations::random_noise(PaletteLavaColors, strip, categoryChange, true,
                               3);
      break;
    }
    case 3: {
      animations::random_noise(PaletteForestColors, strip, categoryChange, true,
                               3);
      break;
    }
    case 4: {
      animations::random_noise(PaletteOceanColors, strip, categoryChange, true,
                               3);
      break;
    }
    case 5: {  // polar light
      animations::mode_2DPolarLights(255, 128, PaletteAuroraColors,
                                     categoryChange, strip);
      break;
    }
    case 6: {
      animations::fire(60, 60, 255, PaletteHeatColors, strip);
      break;
    }
    case 7: {
      animations::mode_sinewave(128, 128, PaletteRainbowColors, strip);
      break;
    }
    case 8: {
      animations::mode_2DDrift(64, 64, PaletteRainbowColors, strip);
      break;
    }
    case 9: {
      animations::mode_2Ddistortionwaves(128, 128, strip);
      break;
    }

    default:  // error
    {
      colorState = 0;
      strip.clear();
      break;
    }
  }

  // reset category change
  categoryChange = false;
}

void party_mode_update() {
  constexpr uint8_t maxPartyState = 2;
  switch (clamp_state_values(colorState, maxPartyState)) {
    case 0:
      static GenerateComplementaryColor complementaryColor =
          GenerateComplementaryColor(0.3);
      if (categoryChange) complementaryColor.reset();

      isFinished = switchMode ? animations::color_wipe_up(
                                    complementaryColor, 500, isFinished, strip)
                              : animations::color_wipe_down(
                                    complementaryColor, 500, isFinished, strip);
      if (isFinished) {
        switchMode = !switchMode;
        complementaryColor.update();  // update color
      }
      break;

    case 1:
      // random solid color
      static GenerateRandomColor randomColor = GenerateRandomColor();
      if (categoryChange) randomColor.reset();

      isFinished =
          animations::double_side_fill(randomColor, 500, isFinished, strip);
      if (isFinished) randomColor.update();  // update color
      break;

    case 2:
      static GenerateComplementaryColor complementaryPingPongColor =
          GenerateComplementaryColor(0.4);
      if (categoryChange) complementaryPingPongColor.reset();

      // ping pong a color for infinity
      isFinished = animations::dot_ping_pong(complementaryPingPongColor, 1000.0,
                                             128, isFinished, strip);
      if (isFinished) complementaryPingPongColor.update();  // update color
      break;

    default:  // error
      colorState = 0;
      strip.clear();
      break;
  }

  // reset category change
  categoryChange = false;
}

void sound_mode_update() {
  constexpr uint8_t maxSoundState = 2;
  switch (clamp_state_values(colorState, maxSoundState)) {
    case 0:  // vue meter
      static GenerateGradientColor redToGreenGradient = GenerateGradientColor(
          LedStrip::Color(0, 255, 0),
          LedStrip::Color(255, 0, 0));  // gradient from red to green
      if (categoryChange) redToGreenGradient.reset();

      microphone::vu_meter(redToGreenGradient, 128, strip);
      break;

      /*case 1:  // pulse soud
        microphone::fftDisplay(128, 128, PaletteHeatColors, categoryChange,
        strip, 255);

        break;*/

    case 1:
      microphone::mode_2DWaverly(128, 64, PalettePartyColors, strip);
      break;

    default:  // error
      colorState = 0;
      strip.clear();
      break;
  }

  // reset category change
  categoryChange = false;
}

void sirenes_mode_update() {
  constexpr uint8_t maxGyroState = 0;
  switch (clamp_state_values(colorState, maxGyroState)) {
    case 0:
      animations::police(500, false, strip);
      break;

    default:  // error
      colorState = 0;
      strip.clear();
      break;
  }

  // reset category change
  categoryChange = false;
}

void color_mode_update() {
  constexpr uint8_t maxColorMode = 4;
  switch (clamp_state_values(colorMode, maxColorMode)) {
    case 0:  // Fixed colors, that the user can change
      gradient_mode_update();
      break;

    case 1:  // slow changing animations, nice to look at
      calm_mode_update();
      break;

    case 2:
      // fast pacing animations
      party_mode_update();
      break;

    case 3:  // sound reacting mode
      sound_mode_update();
      break;

      /*
      case 4:  // diverse alerts (police, firefighters etc)
        sirenes_mode_update();
        break;
      */

    default:
      colorMode = 0;
      break;
  }

  // reset mode change
  modeChange = false;
}

void power_on_sequence() {
  pinMode(LED_POWER_PIN, OUTPUT);
  digitalWrite(LED_POWER_PIN, HIGH);

  // initialize the strip object
  strip.begin();
  strip.clear();
  strip.show();  // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
}

void power_off_sequence() {
  strip.clear();
  strip.show();  // Clear all pixels

  digitalWrite(LED_POWER_PIN, LOW);
  // high drive input (5mA)
  // The only way to discharge the DC-DC pin...
  pinMode(LED_POWER_PIN, OUTPUT_H0H1);
}

void brightness_update(const uint8_t brightness) {
  strip.setBrightness(brightness);
}

void write_parameters() {
  fileSystem::set_value(std::string(colorModeKey), colorMode);
  fileSystem::set_value(std::string(colorStateKey), colorState);
  fileSystem::set_value(std::string(colorCodeIndexForWarmLightKey),
                        colorCodeIndexForWarmLight);
  fileSystem::set_value(std::string(colorCodeIndexForColoredLightKey),
                        colorCodeIndexForColoredLight);
}

void read_parameters() {
  uint32_t mode = 0;
  if (fileSystem::get_value(std::string(colorModeKey), mode)) {
    colorMode = mode;
  }

  uint32_t state = 0;
  if (fileSystem::get_value(std::string(colorStateKey), state)) {
    colorState = state;
  }

  uint32_t warmLight = 0;
  if (fileSystem::get_value(std::string(colorCodeIndexForWarmLightKey),
                            warmLight)) {
    colorCodeIndexForWarmLight = warmLight;
  }

  uint32_t coloredLight = 0;
  if (fileSystem::get_value(std::string(colorCodeIndexForColoredLightKey),
                            coloredLight)) {
    colorCodeIndexForColoredLight = coloredLight;
  }
}

void button_clicked_default(const uint8_t clicks) {
  switch (clicks) {
    case 2:  // 2 clicks: increment color state
      increment_color_state();
      break;

    case 3:  // 3 clicks: decrement color state
      decrement_color_state();
      break;

    case 4:  // 4 clicks: increment color mode
      increment_color_mode();
      break;

    case 5:  // 5 clicks: decrement color mode
      decrement_color_mode();
      break;
  }
}

void button_hold_default(const uint8_t clicks,
                         const bool isEndOfHoldEvent,
                         const uint32_t holdDuration) {

  switch (clicks) {
    case 3: // 3+hold: color wheel forward
      if (isEndOfHoldEvent) {
        lastColorCodeIndex = colorCodeIndex;

      } else {
        constexpr uint32_t colorStepDuration_ms = 6000;
        const uint32_t timeShift =
            (colorStepDuration_ms * lastColorCodeIndex) / 255;
        const uint32_t colorStep =
            (holdDuration + timeShift) % colorStepDuration_ms;
        colorCodeIndex = map(colorStep, 0, colorStepDuration_ms, 0, UINT8_MAX);
      }
      break;

    case 4: // 4+hold: color wheel backward clicks then hold
      if (isEndOfHoldEvent) {
        lastColorCodeIndex = colorCodeIndex;

      } else {
        constexpr uint32_t colorStepDuration_ms = 6000;
        const uint32_t timeShift =
            (colorStepDuration_ms * lastColorCodeIndex) / 255;

        uint32_t buttonHoldDuration = holdDuration;
        if (holdDuration < timeShift) {
          buttonHoldDuration =
              colorStepDuration_ms - (timeShift - holdDuration);
        } else {
          buttonHoldDuration -= timeShift;
        }
        const uint32_t colorStep = buttonHoldDuration % colorStepDuration_ms;
        colorCodeIndex = map(colorStep, 0, colorStepDuration_ms, UINT8_MAX, 0);
      }
      break;
  }
}

bool button_clicked_usermode(const uint8_t clicks) {
  return true;
}

bool button_hold_usermode(const uint8_t clicks,
                          const bool isEndOfHoldEvent,
                          const uint32_t holdDuration) {
  return true;
}

void loop() {
  color_mode_update();
}

bool should_spawn_thread() {
  return true;
}

void user_thread() {
  strip.show();  // show at the end of the loop (only does it if needed)}
}

}  // namespace user
