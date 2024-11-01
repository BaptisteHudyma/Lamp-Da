#include "behavior.h"

#include <cstdint>

#ifdef LMBD_LAMP_TYPE__SIMPLE
#include "src/user/simple/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__CCT
#include "src/user/cct/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "src/user/indexable/functions.h"
#endif

#include "alerts.h"
#include "charger/charger.h"
#include "ext/math8.h"
#include "ext/noise.h"
#include "physical/IMU.h"
#include "physical/MicroPhone.h"
#include "physical/battery.h"
#include "physical/bluetooth.h"
#include "physical/button.h"
#include "physical/fileSystem.h"
#include "physical/led_power.h"
#include "utils/colorspace.h"
#include "utils/constants.h"
#include "utils/utils.h"

static constexpr uint32_t brightnessKey = utils::hash("brightness");

// constantes
static constexpr uint8_t MIN_BRIGHTNESS = 5;
static constexpr uint8_t MAX_BRIGHTNESS = 255;

static uint8_t MaxBrightnessLimit =
    MAX_BRIGHTNESS;  // temporary upper bound for the brightness

// hold the boolean that configures if button's usermode UI is enabled
bool isButtonUsermodeEnabled = false;

// hold the current level of brightness out of the raise/lower animation
uint8_t BRIGHTNESS = 50;  // default start value
uint8_t currentBrightness = 50;

void update_brightness(const uint8_t newBrightness,
                       const bool shouldUpdateCurrentBrightness,
                       const bool isInitialRead) {
  // safety
  if (newBrightness > MaxBrightnessLimit) return;

  if (BRIGHTNESS != newBrightness) {
    BRIGHTNESS = newBrightness;

    // do not call user functions when reading parameters
    if (!isInitialRead) {
      user::brightness_update(newBrightness);
    }

    if (shouldUpdateCurrentBrightness) currentBrightness = newBrightness;
  }
}

void read_parameters() {
  // load values in memory
  fileSystem::load_initial_values();

  uint32_t brightness = 0;
  if (fileSystem::get_value(brightnessKey, brightness)) {
    update_brightness(brightness, true, true);
  }

  user::read_parameters();
}

void write_parameters() {
  fileSystem::clear();

  fileSystem::set_value(isFirstBootKey, 0);
  fileSystem::set_value(brightnessKey, BRIGHTNESS);

  user::write_parameters();

  fileSystem::write_state();
}

static bool isShutdown = true;
bool is_shutdown() { return isShutdown; }

void startup_sequence() {
  isShutdown = true;

  // initialize the battery level
  battery::get_battery_level(true);

  // critical battery level, do not wake up
  if (battery::get_battery_level() <= batteryCritical + 1) {
    // alert user of low battery
    for (uint8_t i = 0; i < 10; i++) {
      button::set_color(utils::ColorSpace::RED);
      delay(100);
      button::set_color(utils::ColorSpace::BLACK);
      delay(100);
    }

    shutdown();
    return;
  }

  // button usermode is always disabled by default
  button_disable_usermode();

  // let the user power on the system
  user::power_on_sequence();

  isShutdown = false;
}

void shutdown() {
  // flag system as powered down
  const bool wasAlreadyShutdown = isShutdown;
  isShutdown = true;

  // button usermode is kept disabled
  button_disable_usermode();

  // deactivate strip power
  pinMode(OUT_BRIGHTNESS, OUTPUT);
  ledpower::write_current(0);  // power down
  delay(10);

  // disable bluetooth, imu and microphone
  microphone::disable();
  imu::disable();
#ifdef USE_BLUETOOTH
  bluetooth::disable_bluetooth();
#endif

  // some actions are to be done only if the system was power-on before
  if (!wasAlreadyShutdown) {
    // save the current config to a file
    // (takes some time so call it when the lamp appear to be shutdown already)
    write_parameters();

    // let the user power off the system
    user::power_off_sequence();
  }

  // deactivate indicators
  button::set_color(utils::ColorSpace::BLACK);

  // do not power down when charger is plugged in
  if (!charger::is_usb_powered()) {
    charger::shutdown();
    digitalWrite(USB_33V_PWR, LOW);

    // wake up from charger plugged in
    // nrf_gpio_cfg_sense_input(g_ADigitalPinMap[CHARGE_OK],
    // NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_HIGH);

    // power down nrf52.
    // on wake up, it'll start back from the setup phase
    systemOff(BUTTON_PIN, 0);
  }
}

void button_disable_usermode() {
  isButtonUsermodeEnabled = false;
}

bool is_button_usermode_enabled() {
  return isButtonUsermodeEnabled;
}

// call when the button is finally release
void button_clicked_callback(const uint8_t consecutiveButtonCheck) {
  if (consecutiveButtonCheck == 0) return;

  // guard blocking other actions than "turning it on" if is_shutdown
  if (is_shutdown()) {
    if (consecutiveButtonCheck == 1) {
      startup_sequence();
    }
    return;
  }

  // extended "button usermode" bypass
  if (isButtonUsermodeEnabled) {
    // user mode may return "True" to skip default action
    if (user::button_clicked_usermode(consecutiveButtonCheck)) {
      return;
    }
  }

  // basic "default" UI:
  //  - 1 click: on/off
  //  - 7+ clicks: shutdown immediately (if DEBUG_MODE wait for watchdog)
  //
  switch (consecutiveButtonCheck) {

    // 1 click: shutdown
    case 1:
      shutdown();
      break;

    // other behaviors
    default:

      // 7+ clicks: force shutdown (or safety reset if DEBUG_MODE)
      if (consecutiveButtonCheck >= 7) {

#ifdef DEBUG_MODE
        // disable charger and wait 5s to be killed by watchdog
        button::set_color(utils::ColorSpace::PINK);
        charger::disable_charge();
        delay(6000);
#endif
        shutdown();
        return;
      }

      user::button_clicked_default(consecutiveButtonCheck);
      break;
  }
}

#define BRIGHTNESS_RAMP_DURATION_MS 2000
static constexpr float brightnessDivider = 1.0 / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

void button_hold_callback(const uint8_t consecutiveButtonCheck,
                          const uint32_t buttonHoldDuration) {
  if (consecutiveButtonCheck == 0) return;

  // compute parameters of the "press-hold" action
  const bool isEndOfHoldEvent = (buttonHoldDuration <= 1);
  const uint32_t holdDuration = (buttonHoldDuration > HOLD_BUTTON_MIN_MS)
      ? (buttonHoldDuration - HOLD_BUTTON_MIN_MS) : 0;

  //
  // "power off" actions
  //    - actions to be performed by user when lamp is turned off
  //    - temporary, as the wake-up path may not enable this to happen
  //    - this could be refactored of actions to be done < 2s after start
  //

  // 3+hold (3s): turn it on, with bluetooth advertising
#ifdef USE_BLUETOOTH
  if (is_shutdown() && consecutiveButtonCheck == 3) {
    if (isEndOfHoldEvent) return;
    if (holdDuration > 3000 - HOLD_BUTTON_MIN_MS) {
      startup_sequence();
      bluetooth::start_advertising();
      return;
    }
  }
#endif

  // 5+hold (3s): turn it on, with button usermode enabled
  if (is_shutdown() && consecutiveButtonCheck == 5) {
    if (isEndOfHoldEvent) return;
    if (holdDuration > 3000 - HOLD_BUTTON_MIN_MS) {
      startup_sequence();

      // (2 pink flashes to confirm)
      for (uint8_t I = 0; I < 2; ++I) {
        button::set_color(utils::ColorSpace::BLACK);
        delay(100);
        button::set_color(utils::ColorSpace::PINK);
        delay(100);
      }

      isButtonUsermodeEnabled = true;
      return;
    }
  }

  if (is_shutdown()) return;

  // extended "button usermode" bypass
  if (isButtonUsermodeEnabled) {

    // 5+hold (5s): always exit, can't be bypassed
    if (consecutiveButtonCheck == 5) {
      if (holdDuration > 5000 - HOLD_BUTTON_MIN_MS) {
        shutdown();
        return;
      }
    }

    // user mode may return "True" to skip default action
    if (user::button_hold_usermode(consecutiveButtonCheck,
                                   isEndOfHoldEvent,
                                   holdDuration)) {
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
  switch (consecutiveButtonCheck) {

    // 1+hold: increase brightness
    case 1:
      if (isEndOfHoldEvent) {
        currentBrightness = BRIGHTNESS;

      } else {
        const float percentOfTimeToGoUp =
            float(MAX_BRIGHTNESS - currentBrightness) * brightnessDivider;

        const auto newBrightness =
            map(min(holdDuration,
                    BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp),
                0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
                currentBrightness, MAX_BRIGHTNESS);

        update_brightness(newBrightness);
      }
      break;

    // 2+hold: decrease brightness
    case 2:
      if (isEndOfHoldEvent) {
        currentBrightness = BRIGHTNESS;

      } else {
        const double percentOfTimeToGoDown =
            float(currentBrightness - MIN_BRIGHTNESS) * brightnessDivider;

        const auto newBrightness =
            map(min(holdDuration,
                    BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown),
                0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown,
                currentBrightness, MIN_BRIGHTNESS);

        update_brightness(newBrightness);
      }
      break;

    // other behaviors
    default:
      user::button_hold_default(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
      break;
  }
}

void handle_alerts() {
  const uint32_t current = AlertManager.current();

  const bool isChargeOk = charger::charge_processus();

  static uint32_t criticalbatteryRaisedTime = 0;
  if (current == Alerts::NONE) {
    MaxBrightnessLimit = MAX_BRIGHTNESS;  // no alerts: reset the max brightness
    criticalbatteryRaisedTime = 0;

    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(
        utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                            utils::ColorSpace::GREEN.get_rgb().color,
                            battery::get_battery_level() / 100.0));

    // display battery level
    if (isChargeOk) {
      // charge mode
      button::breeze(2000, 1000, buttonColor);
    } else {
      // normal mode
      button::set_color(buttonColor);
    }
  } else {
    if ((current & Alerts::TEMP_CRITICAL) != 0x00) {
      // shutdown when battery is critical
      shutdown();
    } else if ((current & Alerts::TEMP_TOO_HIGH) != 0x00) {
      // proc temperature is too high, blink orange
      button::blink(300, 300, utils::ColorSpace::ORANGE);

      // limit brightness to half the max value
      constexpr uint8_t clampedBrightness = 0.5 * MAX_BRIGHTNESS;
      MaxBrightnessLimit = clampedBrightness;
      update_brightness(min(clampedBrightness, BRIGHTNESS));
    } else if ((current & Alerts::BATTERY_READINGS_INCOHERENT) != 0x00) {
      // incohrent battery readings
      button::blink(100, 100, utils::ColorSpace::GREEN);
    } else if ((current & Alerts::BATTERY_CRITICAL) != 0x00) {
      // critical battery alert: shutdown after 2 seconds
      if (criticalbatteryRaisedTime == 0)
        criticalbatteryRaisedTime = millis();
      else if (millis() - criticalbatteryRaisedTime > 2000) {
        // shutdown when battery is critical
        shutdown();
      }
      // blink if no shutdown
      button::blink(100, 100, utils::ColorSpace::RED);
    } else if ((current & Alerts::BATTERY_LOW) != 0x00) {
      criticalbatteryRaisedTime = 0;
      // fast blink red
      button::blink(300, 300, utils::ColorSpace::RED);

      // limit brightness to quarter of the max value
      constexpr uint8_t clampedBrightness = 0.25 * MAX_BRIGHTNESS;
      MaxBrightnessLimit = clampedBrightness;

      // save some battery
      bluetooth::disable_bluetooth();
      update_brightness(min(clampedBrightness, BRIGHTNESS));
    } else if ((current & Alerts::LONG_LOOP_UPDATE) != 0x00) {
      // fast blink red
      button::blink(400, 400, utils::ColorSpace::FUSHIA);
    } else if ((current & Alerts::BLUETOOTH_ADVERT) != 0x00) {
      button::breeze(1000, 500, utils::ColorSpace::BLUE);
    } else {
      // unhandled case (white blink)
      button::blink(300, 300, utils::ColorSpace::WHITE);
    }
  }
}

#ifdef LMBD_CPP17
const char* ensure_build_canary() {
#ifdef LMBD_LAMP_TYPE__SIMPLE
  return "_lmbd__build_canary__simple";
#endif
#ifdef LMBD_LAMP_TYPE__CCT
  return "_lmbd__build_canary__cct";
#endif
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  return "_lmbd__build_canary__indexable";
#endif
  return (char*)nullptr;
}
#endif
