#include "behavior.h"

#include <cstdint>

#include "alerts.h"
#include "charger/charger.h"
#include "ext/math8.h"
#include "ext/noise.h"
#include "physical/IMU.h"
#include "physical/MicroPhone.h"
#include "physical/battery.h"
#include "physical/button.h"
#include "physical/fileSystem.h"
#include "physical/led_power.h"
#include "utils/colorspace.h"
#include "utils/constants.h"
#include "utils/utils.h"

const char* brightnessKey = "brightness";

// constantes
static constexpr uint8_t MIN_BRIGHTNESS = 5;

// hold the current level of brightness out of the raise/lower animation
uint8_t BRIGHTNESS = 50;  // default start value
uint8_t currentBrightness = 50;

void update_brightness(const uint8_t newBrightness) {
  if (BRIGHTNESS != newBrightness) {
    BRIGHTNESS = newBrightness;

    // TODO: update your brightness here
  }
}

void read_parameters() {
  fileSystem::load_initial_values();

  uint32_t brightness = 0;
  if (fileSystem::get_value(std::string(brightnessKey), brightness)) {
    currentBrightness = brightness;
    update_brightness(brightness);
  }
}

void write_parameters() {
  fileSystem::clear();
  fileSystem::set_value(std::string(brightnessKey), BRIGHTNESS);
  fileSystem::write_state();
}

static bool isShutdown = true;
bool is_shutdown() { return isShutdown; }

void startup_sequence() {
  // initialize the battery level
  get_battery_level(true);

  // critical battery level, do not wake up
  if (get_battery_level() <= batteryCritical + 1) {
    // alert user of low battery
    for (uint i = 0; i < 10; i++) {
      set_button_color(utils::ColorSpace::RED);
      delay(100);
      set_button_color(utils::ColorSpace::BLACK);
      delay(100);
    }

    // emergency shutdown
    shutdown();
  }

  Serial.begin(115200);

  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  // activate microphone readings
  // sound::enable_microphone();
#ifdef USE_BLUETOOTH
  bluetooth::enable_bluetooth();
  bluetooth::startup_sequence();
#endif

  isShutdown = false;
  // activate led
  write_brightness(BRIGHTNESS);
  currentBrightness = BRIGHTNESS;
}

void shutdown() {
  // deactivate strip power
  pinMode(OUT_BRIGHTNESS, OUTPUT);
  write_brightness(0);  // power down
  delay(10);

  // disable bluetooth, imu and microphone
  sound::disable_microphone();
  imu::disable_imu();
#ifdef USE_BLUETOOTH
  bluetooth::disable_bluetooth();
#endif

  // save the current config to a file (akes some time so call it when the lamp
  // appear to be shutdown already)
  write_parameters();

  // deactivate indicators
  set_button_color(utils::ColorSpace::RGB(0, 0, 0));

  // do not power down when charger is plugged in
  if (!charger::is_powered_on()) {
    set_wake_up_signal();

    // power down nrf52.
    // If no sense pins are setup (or other hardware interrupts), the nrf52
    // will not wake up.
    sd_power_system_off();  // this function puts the whole nRF52 to deep
                            // sleep.
    // on wake up, it'll start back from the setup phase
  }

  isShutdown = true;
}

// call when the button is finally release
void button_clicked_callback(uint8_t consecutiveButtonCheck) {
  if (consecutiveButtonCheck == 0) return;

  switch (consecutiveButtonCheck) {
    case 1:  // 1 click: shutdown
      if (is_shutdown()) {
        startup_sequence();
      } else {
        shutdown();
      }
      break;

    case 2:
      if (not is_shutdown()) {
        currentBrightness = 255;  // update the slope value
        update_brightness(255);
      }
      break;

    default:
      // unhandled
      break;
  }
}

#define BRIGHTNESS_RAMP_DURATION_MS 4000

void button_hold_callback(uint8_t consecutiveButtonCheck,
                          uint32_t buttonHoldDuration) {
  // no click event
  if (consecutiveButtonCheck == 0) return;

  const bool isEndOfHoldEvent = buttonHoldDuration <= 1;
  buttonHoldDuration -= HOLD_BUTTON_MIN_MS;

  switch (consecutiveButtonCheck) {
    case 1:  // just hold the click
      if (!isEndOfHoldEvent) {
        const float percentOfTimeToGoUp =
            float(255 - currentBrightness) / float(255 - MIN_BRIGHTNESS);

        const auto newBrightness =
            map(min(buttonHoldDuration,
                    BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp),
                0, BRIGHTNESS_RAMP_DURATION_MS * percentOfTimeToGoUp,
                currentBrightness, 255);
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
            float(currentBrightness - MIN_BRIGHTNESS) /
            float(255 - MIN_BRIGHTNESS);

        const auto newBrightness =
            map(min(buttonHoldDuration,
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
      // unhandled
      break;
  }
}

void handle_alerts() {
  const uint32_t current = AlertManager.current();

  static uint32_t criticalbatteryRaisedTime = 0;
  if (current == Alerts::NONE) {
    criticalbatteryRaisedTime = 0;

    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(
        utils::ColorSpace::RED.get_rgb().color,
        utils::ColorSpace::GREEN.get_rgb().color, get_battery_level() / 100.0));

    // display battery level
    if (charger::enable_charge()) {
      // charge mode
      button_breeze(2000, 2000, buttonColor);
    } else {
      // normal mode
      set_button_color(buttonColor);
    }
  } else {
    if ((current & Alerts::BATTERY_READINGS_INCOHERENT) != 0x00) {
      // incohrent battery readings
      button_blink(100, 100, utils::ColorSpace::GREEN);
    } else if ((current & Alerts::BATTERY_CRITICAL) != 0x00) {
      // critical battery alert: shutdown after 2 seconds
      if (criticalbatteryRaisedTime == 0)
        criticalbatteryRaisedTime = millis();
      else if (millis() - criticalbatteryRaisedTime > 2000) {
        // shutdown when battery is critical
        shutdown();
      }
      // blink if no shutdown
      button_blink(100, 100, utils::ColorSpace::RED);
    } else if ((current & Alerts::BATTERY_LOW) != 0x00) {
      criticalbatteryRaisedTime = 0;
      // fast blink red
      button_blink(300, 300, utils::ColorSpace::RED);
    } else if ((current & Alerts::LONG_LOOP_UPDATE) != 0x00) {
      // fast blink red
      button_blink(400, 400, utils::ColorSpace::FUSHIA);
    } else {
      // unhandled case
      button_blink(300, 300, utils::ColorSpace::ORANGE);
    }
  }
}