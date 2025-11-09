#include "src/compile.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/inputs.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/indicator.h"
#include "src/system/physical/imu.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"
#include "src/system/physical/sound.h"

#include "src/system/power/charger.h"
#include "src/system/power/power_handler.h"

#include "src/system/utils/serial.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/sunset_timer.h"

#include "src/user/functions.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"

#include "src/system/ext/random8.h"
#include <cstdint>

namespace global {

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
  if (runTime > MAIN_LOOP_UPDATE_PERIOD_MS + 1)
  {
    isOnSlowLoopCount = min<uint8_t>(isOnSlowLoopCount + 1, maxAlerts);

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
  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  setup_watchdog(10); // second timeout

#ifdef IS_HARDWARE_1_0
  DigitalPin(DigitalPin::GPIO::Input_isChargeOk).set_pin_mode(DigitalPin::Mode::kInputPullUp);
  DigitalPin(DigitalPin::GPIO::Signal_BatteryBalancerAlert).set_pin_mode(DigitalPin::Mode::kInputPullUp);
#endif

  // enable peripherals (enable i2c lines)
  DigitalPin(DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(true);

  // start by resetting the led driver
  outputPower::write_voltage(0);

  // necessary for all i2c communications
  // 400KHz clock, 100mS timeout
  for (uint8_t i = 0; i < get_wire_interface_count(); ++i)
  {
    i2c_setup(i, 400000, 100);
  }
  // stability/turn on delay
  delay_ms(10);

  // first step !
  setup_adc(ADC_RES_EXP);
  // set random seed
  random16_set_seed(get_device_serial_number() & 0xffff);

  // do some stuff before starting the peripherals

  // start the file system
  fileSystem::setup();

  // check if we are in first boot mode (read parameters fails)
  const bool isFirstBoot = not behavior::read_parameters();
#ifdef LMBD_SIMULATION
  fprintf(stderr, "Is first time boot %d\n", isFirstBoot);
#endif

  // can start !

  // setup serial
  serial::setup();

  // setup power components
  power::init();

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

  const bool wasPoweredByUserInterrupt = is_started_from_interrupt();

  // set up button colors and callbacks
  inputs::init(wasPoweredByUserInterrupt);
  imu::init();

  if (shouldAlertUser)
  {
    for (int i = 0; i < 5; i++)
    {
      indicator::set_color(utils::ColorSpace::WHITE);
      delay_ms(300);
      indicator::set_color(utils::ColorSpace::BLACK);
      delay_ms(300);
    }
    indicator::set_color(utils::ColorSpace::BLACK);
  }

  // any wake up from something that is not an interrupt should be considered as vbus voltage
  behavior::set_woke_up_from_vbus(not wasPoweredByUserInterrupt);

  // let the user start in unpowered mode
  user::power_off_sequence();

  // start all power threads
  power::start_threads();
  // start sunset timer thread
  sunset::init();

  // user requested another thread, spawn it
  if (user::should_spawn_thread())
  {
    // give a high stack but low priority to user
    start_thread(secondary_thread, user_taskName, 0, 1024);
  }
}

void regulate_loop_runtime(const uint32_t addedDelay)
{
  // add the required delay
  if (addedDelay > 0)
    delay_ms(addedDelay);

  const uint32_t loopStartTime = time_ms();

  // fix the initialization or long wait
  static uint32_t lastLoopEndTime;
  if (loopStartTime - lastLoopEndTime > 1000)
    lastLoopEndTime = loopStartTime;

  // wait for a delay if we are faster than the set refresh rate
  const uint32_t loopDuration = loopStartTime - lastLoopEndTime;
  if (loopDuration < MAIN_LOOP_UPDATE_PERIOD_MS)
  {
    delay_ms(MAIN_LOOP_UPDATE_PERIOD_MS - loopDuration);
  }
  // else: run time normal or too long

  // raise alerts if computations are too long
  check_loop_runtime(loopDuration);
  // update loop end time
  lastLoopEndTime = time_ms();
}

/**
 * \brief Run the main program loop
 */
void main_loop(const uint32_t addedDelay)
{
  // regulate to a fixed fps count
  regulate_loop_runtime(addedDelay);

  /*
   * Normal loop starts here (all computations)
   */

  // update watchdog (prevent crash)
  kick_watchdog(USER_WATCHDOG_ID);

  // handle inputs
  inputs::loop();

  // handle user serial events
  serial::handleSerialEvents();

  // loop the behavior
  behavior::loop();

  // automatically deactivate sensors if they are not used for a time
  microphone::disable_after_non_use();
}

} // namespace global
