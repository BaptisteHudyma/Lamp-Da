#include "src/compile.h"

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/i2c.h"
#include "src/system/hal/gpio.h"
#include "src/system/hal/time.h"
#include "src/system/hal/registers.h"

#include "src/system/bsp/indicator.h"
#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/command_line_interface.h"
#include "src/system/logic/inputs.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/component/battery.h"
#include "src/system/component/charger.h"
#include "src/system/component/fileSystem.h"
#include "src/system/component/imu.h"
#include "src/system/component/output_power.h"
#include "src/system/component/sound.h"

#include "src/system/utils/utils.h"

#include "src/user/functions.h"

#include "src/system/ext/random8.h"
#include <cstdint>

namespace lampda {

void secondary_thread()
{
  if (not logic::behavior::is_user_code_running())
  {
    bsp::threads::suspend_this_thread();
    return;
  }

  user::user_thread();

  // prevent infinite loop
  hal::delay_ms(1);
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
      hal::registers::enter_serial_dfu();
    }
  }
  else if (isOnSlowLoopCount > 0)
  {
    isOnSlowLoopCount--;
  }

  if (isOnSlowLoopCount >= maxAlerts)
  {
    alarmRaisedTime = hal::time_ms();
    logic::alerts::manager.raise(logic::alerts::Type::LONG_LOOP_UPDATE);
  }
  // lower the alert (after some time)
  else if (isOnSlowLoopCount <= 1 and hal::time_ms() - alarmRaisedTime > 1000)
  {
    logic::alerts::manager.clear(logic::alerts::Type::LONG_LOOP_UPDATE);
  };
}

void main_setup()
{
  // set watchdog (reset the soft when the program crashes)
  // Should be long enough to flash the microcontroler !!!
  hal::registers::setup_watchdog(10); // second timeout

#ifdef IS_HARDWARE_1_0
  hal::gpio::DigitalPin(hal::gpio::DigitalPin::GPIO::Input_isChargeOk)
          .set_pin_mode(hal::gpio::DigitalPin::Mode::kInputPullUp);
  hal::gpio::DigitalPin(hal::gpio::DigitalPin::GPIO::Signal_BatteryBalancerAlert)
          .set_pin_mode(hal::gpio::DigitalPin::Mode::kInputPullUp);
#endif

  // setup button colors and callbacks very early to catch start clics
  const bool wasPoweredByUserInterrupt = hal::registers::is_started_from_interrupt();
  logic::inputs::init(wasPoweredByUserInterrupt);

  // enable peripherals (enable i2c lines)
  hal::gpio::DigitalPin(hal::gpio::DigitalPin::GPIO::Output_EnableExternalPeripherals).set_high(true);

  // reset the output driver
  component::outputPower::write_voltage(0);

  // necessary for all i2c communications
  // 400KHz clock, 100mS timeout
  for (uint8_t i = 0; i < hal::registers::get_wire_interface_count(); ++i)
  {
    hal::i2c::i2c_setup(i, 400000, 100);
  }
  // stability/turn on delay
  hal::delay_ms(10);

  // first step !
  hal::registers::setup_adc(ADC_RES_EXP);
  // set random seed
  random16_set_seed(hal::registers::get_device_serial_number() & 0xffff);

  //
  if (hal::registers::is_started_from_watchdog())
  {
    // try to start fresh: the system can get stuck with a broken filesystem
    // TODO #353: it happens when the system power source is removed during a file system read/write.
    component::fileSystem::clear_internal_fs();
  }

  // check if we are in first boot mode (read parameters fails)
  const bool isFirstBoot = not logic::behavior::read_parameters();
#ifdef LMBD_SIMULATION
  bsp::lampda_print("Is first time boot %d", isFirstBoot);
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
    if (hal::registers::is_started_from_reset())
    {
      hal::registers::enter_serial_dfu();
    }

    if (hal::registers::is_started_from_watchdog())
    {
      // power detected on the USB, reset the program
      if (hal::registers::is_voltage_detected_on_vbus())
      {
        // system will reset & shutdown after that
        hal::registers::enter_serial_dfu();
      }
      else
      {
        // alert the user that the lamp was resetted by watchdog
        shouldAlertUser = true;
      }
    }
  }

  // setup imu
  component::imu::init();

  if (shouldAlertUser)
  {
    for (int i = 0; i < 5; i++)
    {
      bsp::indicator::set_color(utils::ColorSpace::WHITE);
      hal::delay_ms(300);
      bsp::indicator::set_color(utils::ColorSpace::BLACK);
      hal::delay_ms(300);
    }
    bsp::indicator::set_color(utils::ColorSpace::BLACK);
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
    static constexpr uint16_t userThreadBufferSize = 1024;
    static bsp::threads::TaskBuffer_t userThreadBuffer[userThreadBufferSize];
    bsp::threads::start_thread(
            secondary_thread, bsp::threads::user_taskName, 0, userThreadBufferSize, userThreadBuffer);
  }
}

void regulate_loop_runtime(const uint32_t addedDelay)
{
  // add the required delay
  if (addedDelay > 0)
    hal::delay_ms(addedDelay);

  const uint32_t loopStartTime = hal::time_ms();

  // fix the initialization or long wait
  static uint32_t lastLoopEndTime;
  if (loopStartTime - lastLoopEndTime > 1000)
    lastLoopEndTime = loopStartTime;

  // wait for a delay if we are faster than the set refresh rate
  const uint32_t loopDuration = loopStartTime - lastLoopEndTime;
  if (loopDuration < MAIN_LOOP_UPDATE_PERIOD_MS)
  {
    hal::delay_ms(MAIN_LOOP_UPDATE_PERIOD_MS - loopDuration);
  }
  // else: run time normal or too long

  // raise alerts if computations are too long
  check_loop_runtime(loopDuration);
  // update loop end time
  lastLoopEndTime = hal::time_ms();
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
  hal::registers::kick_watchdog(USER_WATCHDOG_ID);

  // handle inputs
  logic::inputs::loop();

  // handle user serial events
  logic::cli::handleSerialEvents();

  // loop the behavior
  logic::behavior::loop();

  // automatically deactivate sensors if they are not used for a time
  component::microphone::disable_after_non_use();
}

} // namespace lampda
