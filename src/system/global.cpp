#include "src/compile.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/command_line_interface.h"
#include "src/system/logic/inputs.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/indicator.h"
#include "src/system/physical/imu.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"
#include "src/system/physical/sound.h"

#include "src/system/power/charger.h"

#include "src/system/utils/utils.h"

#include "src/user/functions.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"

#include "src/system/ext/random8.h"
#include <cstdint>

namespace lampda {

void secondary_thread()
{
  if (not logic::behavior::is_user_code_running())
    return;

  user::user_thread();

  // prevent infinite loop
  platform::delay_ms(1);
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
      platform::registers::enter_serial_dfu();
    }
  }
  else if (isOnSlowLoopCount > 0)
  {
    isOnSlowLoopCount--;
  }

  if (isOnSlowLoopCount >= maxAlerts)
  {
    alarmRaisedTime = platform::time_ms();
    logic::alerts::manager.raise(logic::alerts::Type::LONG_LOOP_UPDATE);
  }
  // lower the alert (after some time)
  else if (isOnSlowLoopCount <= 1 and platform::time_ms() - alarmRaisedTime > 1000)
  {
    logic::alerts::manager.clear(logic::alerts::Type::LONG_LOOP_UPDATE);
  };
}

void main_setup()
{
  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  platform::registers::setup_watchdog(10); // second timeout

#ifdef IS_HARDWARE_1_0
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Input_isChargeOk)
          .set_pin_mode(platform::gpio::DigitalPin::Mode::kInputPullUp);
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Signal_BatteryBalancerAlert)
          .set_pin_mode(platform::gpio::DigitalPin::Mode::kInputPullUp);
#endif

  // enable peripherals (enable i2c lines)
  platform::gpio::DigitalPin(platform::gpio::DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(true);

  // start by resetting the led driver
  physical::outputPower::write_voltage(0);

  // necessary for all i2c communications
  // 400KHz clock, 100mS timeout
  for (uint8_t i = 0; i < platform::registers::get_wire_interface_count(); ++i)
  {
    platform::i2c::i2c_setup(i, 400000, 100);
  }
  // stability/turn on delay
  platform::delay_ms(10);

  // first step !
  platform::registers::setup_adc(ADC_RES_EXP);
  // set random seed
  random16_set_seed(platform::registers::get_device_serial_number() & 0xffff);

  //
  if (platform::registers::is_started_from_watchdog())
  {
    // try to start fresh: the system can get stuck with a broken filesystem
    // TODO: find why !!!!
    physical::fileSystem::clear_internal_fs();
  }

  // check if we are in first boot mode (read parameters fails)
  const bool isFirstBoot = not logic::behavior::read_parameters();
#ifdef LMBD_SIMULATION
  fprintf(stderr, "Is first time boot %d\n", isFirstBoot);
#endif

  // can start !

  // setup command line interface
  logic::cli::setup();

  // setup power components
  logic::power::init();

  bool shouldAlertUser = false;
  // handle start flags
  if (!isFirstBoot)
  {
    // started after reset, clear all code and go to bootloader mode
    if (platform::registers::is_started_from_reset())
    {
      platform::registers::enter_serial_dfu();
    }

    if (platform::registers::is_started_from_watchdog())
    {
      // power detected on the USB, reset the program
      if (platform::registers::is_voltage_detected_on_vbus())
      {
        // system will reset & shutdown after that
        platform::registers::enter_serial_dfu();
      }
      else
      {
        // alert the user that the lamp was resetted by watchdog
        shouldAlertUser = true;
      }
    }
  }

  const bool wasPoweredByUserInterrupt = platform::registers::is_started_from_interrupt();

  // set up button colors and callbacks
  logic::inputs::init(wasPoweredByUserInterrupt);
  physical::imu::init();

  if (shouldAlertUser)
  {
    for (int i = 0; i < 5; i++)
    {
      physical::indicator::set_color(utils::ColorSpace::WHITE);
      platform::delay_ms(300);
      physical::indicator::set_color(utils::ColorSpace::BLACK);
      platform::delay_ms(300);
    }
    physical::indicator::set_color(utils::ColorSpace::BLACK);
  }

  // any wake up from something that is not an interrupt should be considered as vbus voltage
  logic::behavior::set_woke_up_from_vbus(not wasPoweredByUserInterrupt);

  // let the user start in unpowered mode
  user::power_off_sequence();

  // start sunset timer thread
  logic::sunset::init();

  // user requested another thread, spawn it
  if (user::should_spawn_thread())
  {
    // give a high stack but low priority to user
    platform::threads::start_thread(secondary_thread, platform::threads::user_taskName, 0, 1024);
  }
}

void regulate_loop_runtime(const uint32_t addedDelay)
{
  // add the required delay
  if (addedDelay > 0)
    platform::delay_ms(addedDelay);

  const uint32_t loopStartTime = platform::time_ms();

  // fix the initialization or long wait
  static uint32_t lastLoopEndTime;
  if (loopStartTime - lastLoopEndTime > 1000)
    lastLoopEndTime = loopStartTime;

  // wait for a delay if we are faster than the set refresh rate
  const uint32_t loopDuration = loopStartTime - lastLoopEndTime;
  if (loopDuration < MAIN_LOOP_UPDATE_PERIOD_MS)
  {
    platform::delay_ms(MAIN_LOOP_UPDATE_PERIOD_MS - loopDuration);
  }
  // else: run time normal or too long

  // raise alerts if computations are too long
  check_loop_runtime(loopDuration);
  // update loop end time
  lastLoopEndTime = platform::time_ms();
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
  platform::registers::kick_watchdog(USER_WATCHDOG_ID);

  // handle inputs
  logic::inputs::loop();

  // handle user serial events
  logic::cli::handleSerialEvents();

  // loop the behavior
  logic::behavior::loop();

  // automatically deactivate sensors if they are not used for a time
  physical::microphone::disable_after_non_use();
}

} // namespace lampda
