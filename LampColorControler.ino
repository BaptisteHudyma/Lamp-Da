#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>

#include "src/system/alerts.h"
#include "src/system/behavior.h"
#include "src/system/charger/charger.h"
#include "src/system/physical/IMU.h"
#include "src/system/physical/MicroPhone.h"
#include "src/system/physical/battery.h"
#include "src/system/physical/bluetooth.h"
#include "src/system/physical/button.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/led_power.h"
#include "src/system/utils/utils.h"
#include "src/user_functions.h"

void set_watchdog(const uint timeoutDelaySecond) {
  // Configure WDT
  NRF_WDT->CONFIG = 0x01;  // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV = timeoutDelaySecond * 32768 + 1;  // set timeout
  NRF_WDT->RREN = 0x01;      // Enable the RR[0] reload register
  NRF_WDT->TASKS_START = 1;  // Start WDT
}

void setup() {
  // start by resetting the led driver
  ledpower::write_current(0);

  bool shouldAlertUser = false;

  // resetted by watchdog
  if ((readResetReason() & POWER_RESETREAS_DOG_Msk) != 0x00) {
    // allow user to flash the program again, without running the currently
    // stored program
    if (charger::is_usb_powered()) {
      // card will shutdown after that
      enterSerialDfu();
    } else {
      // alert the user that the lamp was resetted by watchdog
      shouldAlertUser = true;
    }
  }

  // set watchdog (reset the soft when the program crashes)
  set_watchdog(5);  // second timeout

  // necessary for all i2c communications
  Wire.begin();

  analogReference(AR_INTERNAL_3_0);  // 3v reference
  analogReadResolution(ADC_RES_EXP);

  // start the file system
  fileSystem::setup();
  read_parameters();  // read from the stored config

  /*
  if (!charger::check_vendor_device_values()) {
    // wrong device id
  }
  */

  // set up button colors and callbacks
  button::init();

  if (shouldAlertUser) {
    for (int i = 0; i < 5; i++) {
      button::set_color(utils::ColorSpace::WHITE);
      delay(300);
      button::set_color(utils::ColorSpace::BLACK);
      delay(300);
    }
  }

  if (!charger::is_powered_on()) {
    startup_sequence();
  } else {
    shutdown();
    charger::enable_charge();
  }

  // user requested another thread, spawn it
  if (user::should_spawn_thread()) {
    Scheduler.startLoop(secondary_thread);
  }
}

void secondary_thread() {
  if (is_shutdown()) return;

  user::user_thread();
}

void check_loop_runtime(const uint32_t runTime) {
  static constexpr uint8_t maxAlerts = 5;
  static uint32_t alarmRaisedTime = 0;
  // check the loop duration
  static uint8_t isOnSlowLoopCount = 0;
  if (runTime > LOOP_UPDATE_PERIOD + 1) {
    isOnSlowLoopCount = min(isOnSlowLoopCount + 1, maxAlerts);
  } else if (isOnSlowLoopCount > 0) {
    isOnSlowLoopCount--;
  }

  if (isOnSlowLoopCount >= maxAlerts) {
    alarmRaisedTime = millis();
    AlertManager.raise_alert(Alerts::LONG_LOOP_UPDATE);
  }
  // lower the alert (after 5 seconds)
  else if (isOnSlowLoopCount <= 1 and millis() - alarmRaisedTime > 3000) {
    AlertManager.clear_alert(Alerts::LONG_LOOP_UPDATE);
  };
}

void loop() {
  uint32_t start = millis();

  // update watchdog (prevent crash)
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;

  // loop is not ran in shutdown mode
  button::handle_events(button_clicked_callback, button_hold_callback);

  if (is_shutdown()) {
    // display alerts if needed
    handle_alerts();

    // charger unplugged, real shutdown
    if (!charger::is_powered_on()) {
      shutdown();
    }

    delay(LOOP_UPDATE_PERIOD);
    return;
  }

  // user loop call
  user::loop();

  // temperature alerts
  const float procTemp = readCPUTemperature();
  if (procTemp >= criticalSystemTemp_c) {
    AlertManager.raise_alert(TEMP_CRITICAL);

  } else if (procTemp >= maxSystemTemp_c) {
    AlertManager.raise_alert(TEMP_TOO_HIGH);
  } else {
    AlertManager.clear_alert(TEMP_TOO_HIGH);
    AlertManager.clear_alert(TEMP_CRITICAL);
  }

  if (!charger::is_powered_on()) {
    // no battery alert when charger is on
    battery::raise_battery_alert();
  } else {
    AlertManager.clear_alert(Alerts::BATTERY_LOW);
    AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);
  }

#ifdef USE_BLUETOOTH
  bluetooth::parse_messages();
#endif

  // wait for a delay if we are faster than the set refresh rate
  uint32_t stop = millis();
  const uint32_t loopDuration = (stop - start);
  if (loopDuration < LOOP_UPDATE_PERIOD) {
    delay(LOOP_UPDATE_PERIOD - loopDuration);
  }

  stop = millis();
  check_loop_runtime(stop - start);

  // update led controler
  ledpower::loop();

  // display alerts if needed
  handle_alerts();

  // automaticaly deactivate sensors if they are not used for a time
  microphone::disable_after_non_use();
  imu::disable_after_non_use();
}
