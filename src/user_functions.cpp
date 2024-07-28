#include "user_functions.h"

#include <cstdint>

#include "system/behavior.h"
#include "system/physical/fileSystem.h"
#include "system/physical/led_power.h"

namespace user {

constexpr int powerPin = AD1;
constexpr int whitePin = AD0;
constexpr int yellowPin = AD2;

const char* colorKey = "color";
uint8_t currentColor = 0;
uint8_t lastColor = 0;

static uint8_t currentBrightness = 0;

void set_color(const uint8_t color) {
  const float reduced = currentBrightness / 255.0;

  uint8_t yellowColor = (UINT8_MAX - color) * reduced;
  uint8_t whiteColor = color * reduced;

  analogWrite(yellowPin, yellowColor);
  analogWrite(whitePin, whiteColor);
}

void power_on_sequence() {
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH);

  pinMode(whitePin, OUTPUT);
  pinMode(yellowPin, OUTPUT);

  analogWrite(yellowPin, 0);
  analogWrite(whitePin, 0);

  ledpower::write_brightness(BRIGHTNESS);
}

void power_off_sequence() {
  analogWrite(yellowPin, 0);
  analogWrite(whitePin, 0);

  digitalWrite(powerPin, LOW);
  // high drive input (5mA)
  // The only way to discharge the DC-DC pin...
  pinMode(powerPin, OUTPUT_H0H1);
}

void brightness_update(const uint8_t brightness) {
  if (brightness == 255) {
    // blip
    currentBrightness = 0;
    set_color(0);
    delay(4);  // blip light off if we reached max level
  }
  currentBrightness = brightness;
  set_color(currentColor);
}

void write_parameters() {
  fileSystem::set_value(std::string(colorKey), currentColor);
}

void read_parameters() {
  uint32_t mode = 0;
  if (fileSystem::get_value(std::string(colorKey), mode)) {
    currentColor = mode;
    lastColor = currentColor;
  }

  currentBrightness = BRIGHTNESS;
}

void button_clicked(const uint8_t clicks) {
  switch (clicks) {
    case 0:
    case 1:
      break;

    case 2:
      // put luminosity to maximum
      update_brightness(255, true);
      break;

    default:
      // nothing
      break;
  }
}

void button_hold(const uint8_t clicks, const bool isEndOfHoldEvent,
                 const uint32_t holdDuration) {
  constexpr uint32_t COLOR_RAMP_DURATION_MS = 4000;
  switch (clicks) {
    case 3:  // 3 clicks and hold
      if (!isEndOfHoldEvent) {
        const float percentOfTimeToGoUp =
            float(UINT8_MAX - lastColor) / (float)UINT8_MAX;
        currentColor = map(
            min(holdDuration, COLOR_RAMP_DURATION_MS * percentOfTimeToGoUp), 0,
            COLOR_RAMP_DURATION_MS * percentOfTimeToGoUp, lastColor, UINT8_MAX);
      } else {
        lastColor = currentColor;
      }
      break;

    case 4:
      if (!isEndOfHoldEvent) {
        const double percentOfTimeToGoDown =
            float(lastColor) / (float)UINT8_MAX;

        currentColor = map(
            min(holdDuration, COLOR_RAMP_DURATION_MS * percentOfTimeToGoDown),
            0, COLOR_RAMP_DURATION_MS * percentOfTimeToGoDown, lastColor, 0);
      } else {
        lastColor = currentColor;
      }
      break;
  }
}

void loop() { set_color(currentColor); }

bool should_spawn_thread() { return false; }

void user_thread() {}

}  // namespace user
