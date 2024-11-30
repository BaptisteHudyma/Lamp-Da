#include <Arduino.h>
#include <bluefruit.h>
#include <Wire.h>

#include "src/compile.h"
#include "src/system/alerts.h"
#include "src/system/behavior.h"
#include "src/system/charger/charger.h"
#include "src/system/physical/battery.h"
#include "src/system/physical/bluetooth.h"
#include "src/system/physical/button.h"
#include "src/system/physical/IMU.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/MicroPhone.h"
#include "src/system/physical/led_power.h"
#include "src/system/utils/serial.h"
#include "src/system/utils/utils.h"
#include "src/user/functions.h"

void set_watchdog(const uint32_t timeoutDelaySecond)
{
  // Configure WDT
  NRF_WDT->CONFIG = 0x01;                        // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV = timeoutDelaySecond * 32768 + 1; // set timeout
  NRF_WDT->RREN = 0x01;                          // Enable the RR[0] reload register
  NRF_WDT->TASKS_START = 1;                      // Start WDT
}

// timestamp of the system wake up
static uint32_t turnOnTime = 0;
void setup()
{
  // start by resetting the led driver
  ledpower::write_current(0);

  // set turn on time
  turnOnTime = millis();

  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  set_watchdog(5); // second timeout

  // necessary for all i2c communications
  Wire.setClock(400000); // 400KHz clock
  Wire.setTimeout(100);  // ms timout
  Wire.begin();

  // setup serial
  serial::setup();

  analogReference(AR_INTERNAL_3_0); // 3v reference
  analogReadResolution(ADC_RES_EXP);

  // setup charger
  charger::setup();

  // start the file system
  fileSystem::setup();
  behavior::read_parameters();

  // check if we are in first boot mode (no first boot flag stored)
  const bool isFirstBoot = !fileSystem::doKeyExists(behavior::isFirstBootKey);

  const auto startReason = readResetReason();
  // POWER_RESETREAS_RESETPIN_Msk: reset from pin-reset detected
  // POWER_RESETREAS_DOG_Msk: reset from watchdog
  // POWER_RESETREAS_SREQ_Msk: reset via soft reset
  // POWER_RESETREAS_LOCKUP_Msk: reset from cpu lockup
  // POWER_RESETREAS_OFF_Msk: wake up from pin interrupt
  // POWER_RESETREAS_LPCOMP_Msk: wake up from analogic pin detect (LPCOMP)
  // POWER_RESETREAS_DIF_Msk: wake up from debug interface
  // POWER_RESETREAS_NFC_Msk: wake from NFC field detection
  // POWER_RESETREAS_VBUS_Msk: wake from vbus high signal

  bool shouldAlertUser = false;
  // handle start flags
  if (!isFirstBoot)
  {
    // started after reset, clear all code and go to bootloader mode
    if ((startReason & POWER_RESETREAS_RESETPIN_Msk) != 0x00)
    {
      enterSerialDfu();
    }

    if ((startReason & POWER_RESETREAS_DOG_Msk) != 0x00)
    {
      // POWER_USBREGSTATUS_OUTPUTRDY_Msk : debounce time passed
      // POWER_USBREGSTATUS_VBUSDETECT_Msk : vbus is detected on usb

      // power detected on the USB, reset the program
      if ((NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00)
      {
        // system will reset & shutdown after that
        enterSerialDfu();
      }
      else
      {
        // alert the user that the lamp was resetted by watchdog
        shouldAlertUser = true;
      }
    }
  }

  // set up button colors and callbacks
  button::init();

  if (shouldAlertUser)
  {
    for (int i = 0; i < 5; i++)
    {
      button::set_color(utils::ColorSpace::WHITE);
      delay(300);
      button::set_color(utils::ColorSpace::BLACK);
      delay(300);
    }
  }

  const bool wasPoweredyByUserInterrupt = (startReason & POWER_RESETREAS_OFF_Msk) != 0x00;
  // any wake up from something that is not an interrupt should be considered as vbus voltage
  behavior::set_woke_up_from_vbus(not wasPoweredyByUserInterrupt);

  // let the user start in unpowered mode
  user::power_off_sequence();

  // use the charging thread !
  Scheduler.startLoop(charging_thread);

  // user requested another thread, spawn it
  if (user::should_spawn_thread())
  {
    Scheduler.startLoop(secondary_thread);
  }
}

void charging_thread()
{
  // run the charger loop (all the time)
  charger::loop();
  delay(2);
}

void secondary_thread()
{
  if (not behavior::is_user_code_running())
    return;

  user::user_thread();
}

void check_loop_runtime(const uint32_t runTime)
{
  static constexpr uint8_t maxAlerts = 5;
  static uint32_t alarmRaisedTime = 0;
  // check the loop duration
  static uint8_t isOnSlowLoopCount = 0;
  if (runTime > LOOP_UPDATE_PERIOD + 1)
  {
    isOnSlowLoopCount = min(isOnSlowLoopCount + 1, maxAlerts);

    if (runTime > 500)
    {
      // if loop time is too long, go back to flash mode
      enterSerialDfu();
    }
  }
  else if (isOnSlowLoopCount > 0)
  {
    isOnSlowLoopCount--;
  }

  if (isOnSlowLoopCount >= maxAlerts)
  {
    alarmRaisedTime = millis();
    AlertManager.raise_alert(Alerts::LONG_LOOP_UPDATE);
  }
  // lower the alert (after 5 seconds)
  else if (isOnSlowLoopCount <= 1 and millis() - alarmRaisedTime > 3000)
  {
    AlertManager.clear_alert(Alerts::LONG_LOOP_UPDATE);
  };
}

/**
 * \brief Run the main program loop
 */
void loop()
{
  uint32_t start = millis();

  // update watchdog (prevent crash)
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;

  // loop is not ran in shutdown mode
  button::handle_events(behavior::button_clicked_callback, behavior::button_hold_callback);

  // handle user serial events
  serial::handleSerialEvents();

  // loop the behavior
  behavior::loop();

  // wait for a delay if we are faster than the set refresh rate
  uint32_t stop = millis();
  const uint32_t loopDuration = (stop - start);
  if (loopDuration < LOOP_UPDATE_PERIOD)
  {
    delay(LOOP_UPDATE_PERIOD - loopDuration);
  }

  stop = millis();
  check_loop_runtime(stop - start);

  // automatically deactivate sensors if they are not used for a time
  microphone::disable_after_non_use();
  imu::disable_after_non_use();
}
