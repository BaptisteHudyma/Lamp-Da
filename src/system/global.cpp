#include "power/power_handler.h"
#include "src/compile.h"

#include "src/system/alerts.h"
#include "src/system/behavior.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"
#include "src/system/physical/IMU.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"
#include "src/system/physical/sound.h"

#include "src/system/power/charger.h"
#include "src/system/power/power_handler.h"

#include "src/system/utils/serial.h"
#include "src/system/utils/utils.h"

#include "src/user/functions.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/time.h"
#include "src/system/platform/registers.h"

namespace global {

// timestamp of the system wake up
static uint32_t turnOnTime = 0;

void charging_thread()
{
  if (behavior::is_shuting_down())
    return;
  power::loop();
  delay_ms(2);
}

void secondary_thread()
{
  if (behavior::is_shuting_down())
    return;
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
  if (runTime > MAIN_LOOP_UPDATE_PERIOD_MS + 1)
  {
    isOnSlowLoopCount = min(isOnSlowLoopCount + 1, maxAlerts);

    if (runTime > 500)
    {
      // if loop time is too long, go back to flash mode
      enter_serial_dfu();
    }
  }
  else if (isOnSlowLoopCount > 0)
  {
    isOnSlowLoopCount--;
  }

  if (isOnSlowLoopCount >= maxAlerts)
  {
    alarmRaisedTime = time_ms();
    alerts::manager.raise(alerts::Type::LONG_LOOP_UPDATE);
  }
  // lower the alert (after some time)
  else if (isOnSlowLoopCount <= 1 and time_ms() - alarmRaisedTime > 1000)
  {
    alerts::manager.clear(alerts::Type::LONG_LOOP_UPDATE);
  };
}

void main_setup()
{
  // enable peripherals (enable i2c lines)
  DigitalPin(DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(true);

  // start by resetting the led driver
  outputPower::write_voltage(0);

  // set turn on time
  turnOnTime = time_ms();

  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  setup_watchdog(5); // second timeout

  // necessary for all i2c communications
  // 400KHz clock, 100mS timeout
  for (uint8_t i = 0; i < get_wire_interface_count(); ++i)
  {
    i2c_setup(i, 400000, 100);
  }
  // stability delay (dont ask...)
  delay_ms(10);

  // setup serial
  serial::setup();

  // first step !
  setup_adc(ADC_RES_EXP);

  // setup power components
  power::init();

  // start the file system
  fileSystem::setup();
  behavior::read_parameters();

  // check if we are in first boot mode (no first boot flag stored)
  const bool isFirstBoot = !fileSystem::doKeyExists(behavior::isFirstBootKey);

  bool shouldAlertUser = false;
  // handle start flags
  if (!isFirstBoot)
  {
    // started after reset, clear all code and go to bootloader mode
    if (is_started_from_reset())
    {
      enter_serial_dfu();
    }

    if (is_started_from_watchdog())
    {
      // power detected on the USB, reset the program
      if (is_voltage_detected_on_vbus())
      {
        // system will reset & shutdown after that
        enter_serial_dfu();
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
  indicator::init();

  if (shouldAlertUser)
  {
    for (int i = 0; i < 5; i++)
    {
      indicator::set_color(utils::ColorSpace::WHITE);
      delay_ms(300);
      indicator::set_color(utils::ColorSpace::BLACK);
      delay_ms(300);
    }
  }

  const bool wasPoweredyByUserInterrupt = is_started_from_interrupt();
  // any wake up from something that is not an interrupt should be considered as vbus voltage
  behavior::set_woke_up_from_vbus(not wasPoweredyByUserInterrupt);

  // let the user start in unpowered mode
  user::power_off_sequence();

  // use the charging thread !
  start_thread(charging_thread);

  // user requested another thread, spawn it
  if (user::should_spawn_thread())
  {
    start_thread(secondary_thread);
  }
}

/**
 * \brief Run the main program loop
 */
void main_loop(const uint32_t addedDelay)
{
  uint32_t start = time_ms();

  // update watchdog (prevent crash)
  kick_watchdog();

  // loop is not ran in shutdown mode
  button::handle_events(behavior::button_clicked_callback, behavior::button_hold_callback);

  // handle user serial events
  serial::handleSerialEvents();

  // loop the behavior
  behavior::loop();

  // add the required delay
  if (addedDelay > 0)
    delay_ms(addedDelay);

  // wait for a delay if we are faster than the set refresh rate
  const uint32_t stop = time_ms();
  const uint32_t loopDuration = (stop - start);
  if (loopDuration < MAIN_LOOP_UPDATE_PERIOD_MS)
  {
    delay_ms(MAIN_LOOP_UPDATE_PERIOD_MS - loopDuration);
  }
  check_loop_runtime(loopDuration);

  // automatically deactivate sensors if they are not used for a time
  microphone::disable_after_non_use();
  imu::disable_after_non_use();
}

} // namespace global