#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

#include "src/system/global.h"

#include "src/system/bsp/threads.h"

#include "src/system/hal/filesystem.h"
#include "src/system/hal/time.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"

// the electrical mock is not part of the simulator library: included the same way as in simulator.h
#include "simulator/electrical/electrical_mock.cpp"

// access simulation states
#include "simulator/include/hardware_influencer.h"
#include "simulator/include/simulator_state.h"

namespace lampda {

using namespace std::chrono_literals;

/**
 * Regression test for #580: a button chain that exits the lockout mode can take longer than the pre output timeout.
 *
 * The behavior state machine is a global that can't be reset, so it can only be started once per process.
 */
TEST(lockout_test, slow_click_chain_exits_lockout_without_error)
{
  // main_setup starts a thread reading the standard input: an open input would block it, and hang the exit of the tests
  ASSERT_NE(std::freopen("/dev/null", "r", stdin), nullptr);

  simulator::mock_registers::shouldStopThreads = false;
  simulator::mock_registers::isDeepSleep = false;
  simulator::globals::state.isButtonPressed = false;

  // healthy battery, nothing plugged on USB
  simulator::mock_battery::voltage = 11.4f;
  simulator::mock_electrical::inputVbusVoltage = 0.0f;
  simulator::mock_registers::cpuTemperature = 50.0f;

  simulator::start_electrical_mock();
  simulator::time_mocks::reset();

  // boots as if woken up by a button press
  main_setup();

  // the system was put to sleep in lockout mode
  logic::alerts::manager.raise(logic::alerts::Type::SYSTEM_IN_LOCKOUT);

  // clicks at a human pace (after the wake up click), way longer than the 1 second allowed to the pre output state
  std::atomic<bool> isClickChainDone = false;
  std::thread clickThread([&]() {
    for (int i = 0; i < 5; ++i)
    {
      std::this_thread::sleep_for(150ms);
      simulator::globals::state.isButtonPressed = true;
      simulator::mock_gpios::update_callbacks();
      std::this_thread::sleep_for(150ms);
      simulator::globals::state.isButtonPressed = false;
      simulator::mock_gpios::update_callbacks();
    }
    isClickChainDone = true;
  });

  // the chain ends at 1.5 s, and is registered 250 ms later
  const uint32_t startTime = hal::time_ms();
  bool wasErrorReached = false;
  bool wasLockoutClearedEarly = false;
  while (hal::time_ms() - startTime < 3000 and not wasErrorReached and not wasLockoutClearedEarly and
         not simulator::mock_registers::isDeepSleep)
  {
    main_loop(0);
    wasErrorReached = logic::behavior::get_state() == "ERROR";
    // lockout must have been kept until the end of the chain
    wasLockoutClearedEarly =
            not isClickChainDone and not logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
  }
  clickThread.join();

  const auto finalState = logic::behavior::get_state();
  const auto errorMessage = logic::behavior::get_error_state_message();
  const bool isStillLocked = logic::alerts::manager.is_raised(logic::alerts::Type::SYSTEM_IN_LOCKOUT);

  // do not leak the system state in the other tests
  logic::alerts::manager.clear(logic::alerts::Type::SYSTEM_IN_LOCKOUT);
  bsp::threads::shutdown();
  hal::filesystem::format_file_system();

  EXPECT_FALSE(wasLockoutClearedEarly);
  EXPECT_FALSE(wasErrorReached) << errorMessage.c_str();
  EXPECT_FALSE(isStillLocked);
  EXPECT_FALSE(simulator::mock_registers::isDeepSleep);
  EXPECT_STREQ(finalState.c_str(), "OUTPUT_LIGHT");
}

} // namespace lampda
