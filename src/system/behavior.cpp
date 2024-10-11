#include "behavior.h"

#include <cstdint>

#include "../user_functions.h"
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

const char* brightnessKey = "brightness";

// constantes
static constexpr uint8_t MIN_BRIGHTNESS = 5;
static constexpr uint8_t MAX_BRIGHTNESS = 255;

static uint8_t MaxBrightnessLimit =
    MAX_BRIGHTNESS;  // temporary upper bound for the brightness

// hold the current level of brightness out of the raise/lower animation
uint8_t BRIGHTNESS = 50;  // default start value
uint8_t currentBrightness = 50;

// hold the boolean that configures if usermode UI is enable for the button
bool isButtonUsermodeEnabled = false;

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
  if (fileSystem::get_value(std::string(brightnessKey), brightness)) {
    update_brightness(brightness, true, true);
  }

  user::read_parameters();
}

void write_parameters() {
  fileSystem::clear();
  fileSystem::set_value(std::string(brightnessKey), BRIGHTNESS);

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

  // usermode UI always disabled by default
  button_disable_usermode();

  // let the user power on the system
  user::power_on_sequence();

  isShutdown = false;
}

void shutdown() {

  // usermode UI always disabled by default
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

  // deactivate indicators
  button::set_color(utils::ColorSpace::BLACK);

  // some actions are to be done only if the system was power-on before
  if(!isShutdown) {

    // save the current config to a file
    // (takes some time so call it when the lamp appear to be shutdown already)
    write_parameters();

    // let the user power off the system
    user::power_off_sequence();
  }

  // flag system as powered down
  isShutdown = true;

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

// called when the button has been pressed N time
//
void button_clicked_callback(const uint8_t consecutiveButtonCheck) {
  if (consecutiveButtonCheck == 0) return;

  // reject other actions than "turning it on" if is_shutdown
  if (is_shutdown()) {
    if (consecutiveButtonCheck == 1) {
      startup_sequence();
    }
    return;
  }

  // extended "usermode" UI bypass:
  if (isButtonUsermodeEnabled) {

    // custom usermode UI (return true to skip default action)
    if (user::button_clicked_usermode(consecutiveButtonCheck)) {
      return;
    }
  }

  // basic "default" UI:
  //    - 1 click: on/off
  //    - 7+ clicks: shutdown immediately (or if DEBUG_MODE wait watchdog)
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
        button_disable_usermode();

#ifdef DEBUG_MODE
        // disable charger then wait 5s to be killed by watchdog
        button::set_color(utils::ColorSpace::PINK);
        charger::disable_charge();
        delay(6000);
#endif

        shutdown();
        return;
      }

      // user behaviors
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
  // "power-off" actions
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

  // 5+hold (3s): turn it on, but with usermode UI enabled
  if (is_shutdown() && consecutiveButtonCheck == 5) {
    if (isEndOfHoldEvent) return;
    if (holdDuration > 3000 - HOLD_BUTTON_MIN_MS) {
        startup_sequence();

        for (uint8_t I = 0; I < 3; ++I) {
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

  //
  // "power-on" actions
  //

  // extended "usermode" UI called if enabled
  if (isButtonUsermodeEnabled) {

    // 5+hold (3s): always exit, can't be hooked
    if (consecutiveButtonCheck == 5) {
      if (holdDuration > 3000 - HOLD_BUTTON_MIN_MS) {
        shutdown();
        return;
      }
    }

    // custom usermode UI (return true to skip default action)
    if (user::button_hold_usermode(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration)) {
      return;
    }
  }

  // basic "default" UI:
  //  - 1+hold: increase brightness
  //  - 2+hold: decrease brightness
  //
  switch (consecutiveButtonCheck) {

    // 1+hold: larger luminosity
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

    // user behaviors
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
    MaxBrightnessLimit = MAX_BRIGHTNESS;
    criticalbatteryRaisedTime = 0;

    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(
        utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                            utils::ColorSpace::GREEN.get_rgb().color,
                            battery::get_battery_level() / 100.0));

    // display battery level
    if (isChargeOk) {
      // charge mode
      button::breeze(2000, 2000, buttonColor);
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
