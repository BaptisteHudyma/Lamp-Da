#include "behavior.h"

#include <cstdint>

#ifdef LMBD_LAMP_TYPE__SIMPLE
#include "../user/simple/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__CCT
#include "../user/cct/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "../user/indexable/functions.h"
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

const char* brightnessKey = "brightness";

// constantes
static constexpr uint8_t MIN_BRIGHTNESS = 5;
static constexpr uint8_t MAX_BRIGHTNESS = 255;

static uint8_t MaxBrightnessLimit =
    MAX_BRIGHTNESS;  // temporary upper bound for the brightness

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

  // let the user power on the system
  user::power_on_sequence();

  isShutdown = false;
}

void shutdown() {

  // flag system as powered down
  const bool wasAlreadyShutdown = isShutdown;
  isShutdown = true;

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
  if(!wasAlreadyShutdown) {

    // save the current config to a file
    // (takes some time so call it when the lamp appear to be shutdown already)
    write_parameters();

    // let the user power off the system
    user::power_off_sequence();
  }

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

// call when the button is finally release
void button_clicked_callback(const uint8_t consecutiveButtonCheck) {
  if (consecutiveButtonCheck == 0) return;

  switch (consecutiveButtonCheck) {
    case 1:  // 1 click: shutdown
      if (is_shutdown()) {
        startup_sequence();
      } else {
        shutdown();
      }
      break;

    // enable bluetooth
    case 5:
#ifdef USE_BLUETOOTH
      bluetooth::start_advertising();
#endif
      break;

#ifdef DEBUG_MODE
    // force a safety reset of the program
    case 6:
      button::set_color(utils::ColorSpace::PINK);
      // disable charger if charge was enabled
      charger::disable_charge();

      // make watchdog stop the execution
      delay(6000);
      break;
#endif

    default:
      if (!is_shutdown()) {
        // user behavior
        user::button_clicked(consecutiveButtonCheck);
      }
      break;
  }
}

#define BRIGHTNESS_RAMP_DURATION_MS 2000

void button_hold_callback(const uint8_t consecutiveButtonCheck,
                          const uint32_t buttonHoldDuration) {
  // no click event
  if (consecutiveButtonCheck == 0) return;

  // no events when shutdown
  if (is_shutdown()) return;

  const bool isEndOfHoldEvent = buttonHoldDuration <= 1;
  const uint32_t holdDuration = buttonHoldDuration - HOLD_BUTTON_MIN_MS;

  static constexpr float brightnessDivider =
      1.0 / float(MAX_BRIGHTNESS - MIN_BRIGHTNESS);

  switch (consecutiveButtonCheck) {
    case 1:  // just hold the click
      if (!isEndOfHoldEvent) {
        const float percentOfTimeToGoUp =
            float(MAX_BRIGHTNESS - currentBrightness) * brightnessDivider;

        const auto newBrightness =
            map(min(holdDuration,
                    BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp),
                0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
                currentBrightness, MAX_BRIGHTNESS);
        update_brightness(newBrightness);
      } else {
        // switch brightness
        currentBrightness = BRIGHTNESS;
      }
      break;

    case 2:  // 2 click and hold
             // lower luminositity
      if (!isEndOfHoldEvent) {
        const double percentOfTimeToGoDown =
            float(currentBrightness - MIN_BRIGHTNESS) * brightnessDivider;

        const auto newBrightness =
            map(min(holdDuration,
                    BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown),
                0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoDown,
                currentBrightness, MIN_BRIGHTNESS);
        update_brightness(newBrightness);
      } else {
        // switch brightness
        currentBrightness = BRIGHTNESS;
      }
      break;

    default:
      // user defined behavior
      user::button_hold(consecutiveButtonCheck, isEndOfHoldEvent, holdDuration);
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
