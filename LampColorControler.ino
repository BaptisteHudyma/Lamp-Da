#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>

#include "src/compile.h"
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
#include "src/system/utils/serial.h"
#include "src/system/utils/utils.h"

#ifdef LMBD_LAMP_TYPE__SIMPLE
#include "src/user/simple/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__CCT
#include "src/user/cct/functions.h"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "src/user/indexable/functions.h"
#endif

void set_watchdog(const uint timeoutDelaySecond) {
  // Configure WDT
  NRF_WDT->CONFIG = 0x01;  // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV = timeoutDelaySecond * 32768 + 1;  // set timeout
  NRF_WDT->RREN = 0x01;      // Enable the RR[0] reload register
  NRF_WDT->TASKS_START = 1;  // Start WDT
}

// if the system waked up by USB plugged in, set this flag
bool wokeUpFromVBUS = false;

// timestamp of the system wake up
static uint32_t turnOnTime = 0;

void setup() {
  // start by resetting the led driver
  ledpower::write_current(0);

  // set turn on time
  turnOnTime = millis();

  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  set_watchdog(5);  // second timeout

  // necessary for all i2c communications
  Wire.setClock(400000);  // 400KHz clock
  Wire.setTimeout(100);   // ms timout
  Wire.begin();

  // setup serial
  serial::setup();

  analogReference(AR_INTERNAL_3_0);  // 3v reference
  analogReadResolution(ADC_RES_EXP);

  // setup charger
  charger::setup();

  // start the file system
  fileSystem::setup();

  // check if we are in first boot mode
  uint32_t isFirstBoot = 1;
  fileSystem::get_value(std::string("ifb"), isFirstBoot);

  bool shouldAlertUser = false;
  // resetted by watchdog
  if (!isFirstBoot) {
    // started after reset, clear all code and go to bootloader mode
    if ((readResetReason() & POWER_RESETREAS_RESETPIN_Msk) != 0x00) {
      enterSerialDfu();
    }

    if ((readResetReason() & POWER_RESETREAS_DOG_Msk) != 0x00) {
      // allow user to flash the program again, without running the currently
      // stored program
      if (charger::is_usb_powered()) {
        // system will reset & shutdown after that
        enterSerialDfu();
      } else {
        // alert the user that the lamp was resetted by watchdog
        shouldAlertUser = true;
      }
    }
  }

  // read from the stored config
  read_parameters();
  if (isFirstBoot) {
    // if first boot, then store the flag
    fileSystem::set_value(std::string("ifb"), 0);
  }

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

  // started by interrupt (user)
  if ((readResetReason() & POWER_RESETREAS_OFF_Msk) != 0x00) {
    startup_sequence();
  }
  // else: start in shutdown mode
  else {
    wokeUpFromVBUS = true;

    // let the user start in unpowered mode (edge cases...)
    user::power_off_sequence();
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

void cpuTemperarureAlerts() {
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
}

void batteryAlerts() {
  if (!charger::is_powered_on()) {
    // no battery alert when charger is on
    battery::raise_battery_alert();
  } else {
    AlertManager.clear_alert(Alerts::BATTERY_LOW);
    AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);
  }
}

void loop() {
  uint32_t start = millis();

  // update watchdog (prevent crash)
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;

  // loop is not ran in shutdown mode
  button::handle_events(button_clicked_callback, button_hold_callback);
  // handle user serial events
  serial::handleSerialEvents();

  if (is_shutdown()) {
    // display alerts if needed
    handle_alerts();

    // charger unplugged, real shutdown
    if (!charger::is_usb_powered()) {
      if (wokeUpFromVBUS && millis() - turnOnTime < 2000) {
        // security for charger with ringing voltage levels, wait some time for
        // it to settle before shuting down
      } else
        shutdown();
    }

    delay(LOOP_UPDATE_PERIOD);
    return;
  }

  // user loop call
  user::loop();

  cpuTemperarureAlerts();
  batteryAlerts();

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

  // automatically deactivate sensors if they are not used for a time
  microphone::disable_after_non_use();
  imu::disable_after_non_use();
}
