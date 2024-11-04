#ifdef LMBD_LAMP_TYPE__CCT

#include <cstdint>

#include "src/system/behavior.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/utils/utils.h"

#include "src/user/functions.h"

namespace user {

constexpr int powerPin = AD1;
constexpr int whitePin = AD0;
constexpr int yellowPin = AD2;

constexpr uint32_t colorKey = utils::hash("color");
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
  pinMode(yellowPin, OUTPUT);
  pinMode(whitePin, OUTPUT);

  currentBrightness = BRIGHTNESS;
  set_color(currentColor);

  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH);
}

void power_off_sequence() {
  // high drive input (5mA)
  // The only way to discharge the DC-DC pin...
  pinMode(powerPin, OUTPUT_H0H1);
  // turn off 12V driver
  digitalWrite(powerPin, LOW);

  delay(5);
  // reset the output
  currentBrightness = 0;
  set_color(0);

#ifdef LMBD_CPP17
  ensure_build_canary();  // (no-op) internal symbol used during build
#endif
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

void write_parameters() { fileSystem::set_value(colorKey, currentColor); }

void read_parameters() {
  uint32_t mode = 0;
  if (fileSystem::get_value(colorKey, mode)) {
    currentColor = mode;
    lastColor = currentColor;
  }

  currentBrightness = BRIGHTNESS;
}

void button_clicked_default(const uint8_t clicks) {
  switch (clicks) {

    // put luminosity to maximum
    case 2:
      update_brightness(255, true);
      break;

    default:
      break;
  }
}

void button_hold_default(const uint8_t clicks,
                         const bool isEndOfHoldEvent,
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

bool button_clicked_usermode(const uint8_t) {
  return usermodeDefaultsToLockdown;
}

bool button_hold_usermode(const uint8_t, const bool, const uint32_t) {
  return usermodeDefaultsToLockdown;
}

void loop() { set_color(currentColor); }

bool should_spawn_thread() { return false; }

void user_thread() {}

}  // namespace user

#endif
