#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>

#include "src/system/alerts.h"
#include "src/system/behavior.h"
#include "src/system/charger/charger.h"
#include "src/system/physical/MicroPhone.h"
#include "src/system/physical/battery.h"
#include "src/system/physical/bluetooth.h"
#include "src/system/physical/button.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/utils/utils.h"
#include "src/user_functions.h"

void setup() {
  // Necessary for sleep mode for some reason ??
  Bluefruit.begin();
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

  if (!charger::is_powered_on()) {
    startup_sequence();
  } else {
    shutdown();
    charger::enable_charge();
  }
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

  // display alerts if needed
  handle_alerts();
}
