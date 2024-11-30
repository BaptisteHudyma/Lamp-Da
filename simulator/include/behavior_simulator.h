#ifndef BEHAVIOR_SIMULATOR_H
#define BEHAVIOR_SIMULATOR_H

// prevent inclusion of colliding src/system/behavior.h
#define BEHAVIOR_HPP

void pinMode(auto, auto) {}
#define OUTPUT             0x1021
#define OUTPUT_H0H1        0x1022
#define INPUT_PULLUP_SENSE 0x1023
#define CHANGE             0x1024
#define LOW                0x1025
#define HIGH               0x1026
#define LED_POWER_PIN      0x1027

void ensure_build_canary() {}
int digitalPinToInterrupt(auto) { return 0; }
void attachInterrupt(auto, auto, auto) {}
int digitalRead(auto) { return 0; }
void digitalWrite(auto, auto) {}
void analogWrite(auto, auto) {}

#define UTILS_H
#define CONSTANTS_H
#define COLOR_SPACE_H

#include "src/system/physical/button.cpp"

namespace behavior {

static bool isButtonUsermodeEnabled = false;
static uint32_t lastStartupSequence = 0;
static bool isShutdown = true;
static bool isBluetoothAdvertising = false;
static uint8_t currentBrightness = 255;
static uint8_t BRIGHTNESS = 255;
static uint8_t MaxBrightnessLimit = 255;

#define BRIGHTNESS_RAMP_DURATION_MS 2000
#define MIN_BRIGHTNESS              5
#define MAX_BRIGHTNESS              255
#define EARLY_ACTIONS_LIMIT_MS      2000
#define EARLY_ACTIONS_HOLD_MS       2000

static bool deferredBrightnessCallback = false;

void update_brightness(uint8_t newBrightness, bool shouldUpdateCurrentBrightness, bool isInitialRead)
{
  if (newBrightness > MaxBrightnessLimit)
    return;

  if (BRIGHTNESS != newBrightness)
  {
    BRIGHTNESS = newBrightness;

    if (!isInitialRead)
    {
      fprintf(stderr, "re-entrant brightness callback deferred\n");
      deferredBrightnessCallback = true;
    }

    if (shouldUpdateCurrentBrightness)
      currentBrightness = newBrightness;
  }
}

void power_on_behavior(auto& simu)
{
  isShutdown = false;
  lastStartupSequence = millis();
  isButtonUsermodeEnabled = false;
  fprintf(stderr, "startup\n");

  simu.power_on_sequence();
  simu.read_parameters();
}

void power_off_behavior(auto& simu)
{
  isShutdown = true;
  fprintf(stderr, "shutdown\n");

  simu.write_parameters();
  simu.power_off_sequence();
}

void click_behavior(auto& simu, uint8_t consecutiveButtonCheck)
{
  if (consecutiveButtonCheck == 0)
    return;
  fprintf(stderr, "clicked %d\n", consecutiveButtonCheck);

  // guard blocking other actions than "turning it on" if is_shutdown
  if (isShutdown)
  {
    if (consecutiveButtonCheck == 1)
    {
      power_on_behavior(simu); // fake start
    }
    return;
  }

  // extended "button usermode" bypass
  if (isButtonUsermodeEnabled)
  {
    // user mode may return "True" to skip default action
    if (simu.button_clicked_usermode(consecutiveButtonCheck))
    {
      return;
    }
  }

  // basic "default" UI:
  //  - 1 click: on/off
  //  - 7+ clicks: shutdown immediately (if DEBUG_MODE wait for watchdog)
  //
  switch (consecutiveButtonCheck)
  {
    // 1 click: shutdown
    case 1:
      power_off_behavior(simu); // fake stop
      break;

    // other behaviors
    default:

      // 7+ clicks: force shutdown (or safety reset if DEBUG_MODE)
      if (consecutiveButtonCheck >= 7)
      {
        fprintf(stderr, "(force-shutdown)\n");
        power_off_behavior(simu); // fake stop
        return;
      }

      simu.button_clicked_default(consecutiveButtonCheck);
      break;
  }
}

static constexpr float brightnessDivider = 1.0 / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

void hold_behavior(auto& simu, uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration)
{
  if (consecutiveButtonCheck == 0)
    return;
  if (isShutdown)
    return;

  fprintf(stderr, "hold %d %d\n", consecutiveButtonCheck, buttonHoldDuration);

  const bool isEndOfHoldEvent = (buttonHoldDuration <= 1);
  const uint32_t holdDuration =
          (buttonHoldDuration > HOLD_BUTTON_MIN_MS) ? (buttonHoldDuration - HOLD_BUTTON_MIN_MS) : 0;

  uint32_t realStartTime = millis() - lastStartupSequence;
  if (realStartTime > holdDuration)
  {
    realStartTime -= holdDuration;
  }

  if (realStartTime < EARLY_ACTIONS_LIMIT_MS)
  {
    if (isEndOfHoldEvent && consecutiveButtonCheck > 2)
      return;

    // 3+hold (3s): turn it on, with button usermode enabled
    if (consecutiveButtonCheck == 3)
    {
      if (holdDuration > EARLY_ACTIONS_HOLD_MS)
      {
        fprintf(stderr, "button usermode enabled\n");
        isButtonUsermodeEnabled = true;
        return;
      }
    }

    // 4+hold (3s): turn it on, with bluetooth advertising
    if (consecutiveButtonCheck == 4)
    {
      if (holdDuration > EARLY_ACTIONS_HOLD_MS)
      {
        if (!isBluetoothAdvertising)
        {
          fprintf(stderr, "bluetooth advertising\n");
        }

        isBluetoothAdvertising = true;
        return;
      }
      else
      {
        isBluetoothAdvertising = false;
      }
    }
  }

  //
  // "button usermode" bypass
  //

  if (isButtonUsermodeEnabled)
  {
    // 5+hold (5s): always exit, can't be bypassed
    if (consecutiveButtonCheck == 5)
    {
      if (holdDuration > 5000 - HOLD_BUTTON_MIN_MS)
      {
        power_off_behavior(simu);
        return;
      }
    }

    // user mode may return "True" to skip default action
    if (simu.button_hold_usermode(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration))
    {
      return;
    }
  }

  //
  // default actions
  //

  // basic "default" UI:
  //  - 1+hold: increase brightness
  //  - 2+hold: decrease brightness
  //
  switch (consecutiveButtonCheck)
  {
    // 1+hold: increase brightness
    case 1:
      if (isEndOfHoldEvent)
      {
        currentBrightness = BRIGHTNESS;
        fprintf(stderr, "brightness set to %d\n", BRIGHTNESS);
      }
      else
      {
        const float percentOfTimeToGoUp = float(MAX_BRIGHTNESS - currentBrightness) * brightnessDivider;
        if (percentOfTimeToGoUp == 0)
          return;

        const float newBrightness = map(min(holdDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp),
                                        0,
                                        BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
                                        currentBrightness,
                                        MAX_BRIGHTNESS);

        BRIGHTNESS = newBrightness;
        fprintf(stderr, "brightness up %f\n", newBrightness);

        simu.brightness_update(newBrightness);
      }
      break;

    // 2+hold: decrease brightness
    case 2:
      if (isEndOfHoldEvent)
      {
        currentBrightness = BRIGHTNESS;
        fprintf(stderr, "brightness set to %d\n", BRIGHTNESS);
      }
      else
      {
        const double percentOfTimeToGoDown = float(currentBrightness - MIN_BRIGHTNESS) * brightnessDivider;
        if (percentOfTimeToGoDown == 0)
          return;

        const float newBrightness = map(min(holdDuration, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown),
                                        0,
                                        BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown,
                                        currentBrightness,
                                        MIN_BRIGHTNESS);

        BRIGHTNESS = newBrightness;
        fprintf(stderr, "brightness down %f\n", newBrightness);

        simu.brightness_update(newBrightness);
      }
      break;

    // other behaviors
    default:
      simu.button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
      break;
  }
}
} // namespace behavior

#endif
