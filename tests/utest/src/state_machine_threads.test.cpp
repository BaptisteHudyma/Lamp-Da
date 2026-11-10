#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>

#include "src/system/common/state_machine.h"

// access simulation states
#include "simulator/include/hardware_influencer.h"

namespace lampda::common {

namespace {

enum class State
{
  Idle,
  ClearRails,
  Charging,
  Output,
  Error
};

/// long enough to never time out during a test
static constexpr uint32_t longTimeout_ms = 100000;

/// Run machine.run() on its own thread, as the power thread does. Stopped and joined on destruction
class RunThread
{
public:
  explicit RunThread(StateMachine<State>& machine) :
    _thread([this, &machine]() {
      while (not _shouldStop)
        machine.run();
    })
  {
  }
  ~RunThread() { stop(); }

  RunThread(const RunThread&) = delete;
  RunThread& operator=(const RunThread&) = delete;

  void stop()
  {
    _shouldStop = true;
    if (_thread.joinable())
      _thread.join();
  }

private:
  std::atomic<bool> _shouldStop {false};
  // last: started once the other members are initialized
  std::thread _thread;
};

} // namespace

class state_machine_threads_test : public ::testing::Test
{
protected:
  void SetUp() override { ::simulator::time_mocks::reset(); }
  void TearDown() override { ::simulator::time_mocks::reset(); }
};

/**
 * Issue #615: a state set with a timeout, while another thread runs the timeout check, must never reach the previous
 * timeout target (the timeout was seen active before its delay and target were set).
 */
TEST_F(state_machine_threads_test, timeout_set_never_fires_previous_target)
{
  StateMachine<State> machine(State::Idle);
  RunThread runThread(machine);

  for (uint32_t i = 0; i < 50000; ++i)
  {
    // alternate the targets: the previous target is always the other one
    const State target = (i % 2 == 0) ? State::Charging : State::Output;
    machine.set_state(State::ClearRails, longTimeout_ms, target);
    ASSERT_EQ(machine.get_state(), State::ClearRails) << "timed out at once to the previous target, iteration " << i;
    ASSERT_EQ(machine.after_timeout_state(), target) << "iteration " << i;
  }

  runThread.stop();
}

/// The timeout still fires normally with a concurrent timeout check
TEST_F(state_machine_threads_test, timeout_fires_to_new_target)
{
  StateMachine<State> machine(State::Idle);
  machine.set_state(State::ClearRails, longTimeout_ms, State::Charging);
  machine.set_state(State::ClearRails, 20, State::Output);

  RunThread runThread(machine);
  const uint32_t start_ms = hal::time_ms();
  while (machine.get_state() == State::ClearRails)
  {
    ASSERT_LT(hal::time_ms() - start_ms, 2000u) << "timeout never fired";
    std::this_thread::yield();
  }
  runThread.stop();

  EXPECT_EQ(machine.get_state(), State::Output);
  EXPECT_TRUE(machine.state_changed_with_timeout());
}

/// A skip decided before another thread set a new target is dropped
TEST_F(state_machine_threads_test, skip_timeout_dropped_after_new_target)
{
  StateMachine<State> machine(State::Idle);
  machine.set_state(State::ClearRails, longTimeout_ms, State::Charging);
  const uint32_t generation = machine.get_state_generation();

  // the main loop changes the target meanwhile
  machine.set_state(State::ClearRails, longTimeout_ms, State::Output);

  EXPECT_FALSE(machine.skip_timeout(generation));
  EXPECT_EQ(machine.get_state(), State::ClearRails);
  EXPECT_EQ(machine.after_timeout_state(), State::Output);

  // a skip decided on the current target goes through
  EXPECT_TRUE(machine.skip_timeout(machine.get_state_generation()));
  EXPECT_EQ(machine.get_state(), State::Output);
}

/// No timeout: nothing to skip
TEST_F(state_machine_threads_test, skip_timeout_without_timeout)
{
  StateMachine<State> machine(State::Idle);
  EXPECT_FALSE(machine.skip_timeout(machine.get_state_generation()));
  EXPECT_EQ(machine.get_state(), State::Idle);
}

/// Every state set changes the generation, a timeout target change included
TEST_F(state_machine_threads_test, generation_changes_on_every_set)
{
  StateMachine<State> machine(State::Idle);
  uint32_t generation = machine.get_state_generation();

  machine.set_state(State::ClearRails, longTimeout_ms, State::Charging);
  EXPECT_NE(machine.get_state_generation(), generation);
  generation = machine.get_state_generation();

  machine.set_state(State::ClearRails, longTimeout_ms, State::Output);
  EXPECT_NE(machine.get_state_generation(), generation);
  generation = machine.get_state_generation();

  // same state and target: no update
  machine.set_state(State::ClearRails, longTimeout_ms, State::Output);
  EXPECT_EQ(machine.get_state_generation(), generation);

  machine.set_state(State::Idle);
  EXPECT_NE(machine.get_state_generation(), generation);
}

/// An error set by another thread can not be overwritten by a state switch
TEST_F(state_machine_threads_test, set_state_unless_blocked)
{
  StateMachine<State> machine(State::Idle);
  EXPECT_TRUE(machine.set_state_unless(State::Error, State::ClearRails, longTimeout_ms, State::Charging));
  EXPECT_EQ(machine.get_state(), State::ClearRails);

  machine.set_state(State::Error);
  EXPECT_FALSE(machine.set_state_unless(State::Error, State::ClearRails, longTimeout_ms, State::Output));
  EXPECT_EQ(machine.get_state(), State::Error);
}

/// Switches from one thread, errors from another: once in error, the machine stays in error
TEST_F(state_machine_threads_test, error_never_overwritten_by_concurrent_switch)
{
  for (int i = 0; i < 200; ++i)
  {
    StateMachine<State> machine(State::Idle);
    std::atomic<bool> isErrorSet {false};

    std::thread switcher([&]() {
      while (not isErrorSet)
        machine.set_state_unless(State::Error, State::ClearRails, longTimeout_ms, (i % 2) ? State::Charging : State::Output);
      // a few more switches after the error
      for (int j = 0; j < 100; ++j)
        machine.set_state_unless(State::Error, State::ClearRails, longTimeout_ms, State::Charging);
    });

    machine.set_state(State::Error);
    isErrorSet = true;
    switcher.join();

    ASSERT_EQ(machine.get_state(), State::Error) << "iteration " << i;
  }
}

} // namespace lampda::common
