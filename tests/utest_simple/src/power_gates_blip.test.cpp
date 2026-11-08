#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include "src/system/bsp/power_gates.h"
#include "src/system/hal/gpio.h"
#include "src/system/hal/time.h"

// access simulation states
#include "simulator/include/hardware_influencer.h"

namespace lampda {

namespace {

namespace powergates = bsp::powergates;
namespace power = bsp::powergates::power;

/// longest blip accepted by the power gates
static constexpr uint32_t blipMaxDuration_ms = 10000;
/// blip duration, long enough to keep a margin around the clock wrap and the test timings
static constexpr uint32_t blipDuration_ms = 500;
/// the wrap test clock starts this long before its wrap: the gate switch delay must end before the wrap
static constexpr uint32_t timeBeforeWrap_ms = 200;
/// more than the switch delay of the power gates
static constexpr uint32_t gateSwitchWait_ms = 60;

/// Read the output gate pin, as driven by the power gates
bool is_output_gate_pin_high()
{
  static const hal::gpio::DigitalPin outputGate(hal::gpio::DigitalPin::GPIO::Output_EnableOutputGate);
  return outputGate.is_high();
}

/// Read the vbus gate pin, as driven by the power gates
bool is_vbus_gate_pin_high()
{
  static const hal::gpio::DigitalPin vbusGate(hal::gpio::DigitalPin::GPIO::Output_EnableVbusGate);
  return vbusGate.is_high();
}

/// Elapsed time since \p start_ms, safe across the clock wrap
uint32_t elapsed_since(const uint32_t start_ms) { return hal::time_ms() - start_ms; }

/// Wait until \p elapsed_ms passed since \p start_ms
void wait_elapsed(const uint32_t start_ms, const uint32_t elapsed_ms)
{
  while (elapsed_since(start_ms) < elapsed_ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

/// Wait for \p duration_ms, running the power gates loop meanwhile
void run_loop_for(const uint32_t duration_ms)
{
  const uint32_t start_ms = hal::time_ms();
  while (elapsed_since(start_ms) < duration_ms)
  {
    powergates::loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

/// Switch the power gate on, and wait until it is really on
void enable_power_gate_and_wait()
{
  powergates::enable_power_gate();
  const uint32_t start_ms = hal::time_ms();
  wait_elapsed(start_ms, gateSwitchWait_ms);
  powergates::loop();
}

/**
 * Run the power gates loop on its own thread, as the power thread does.
 * Stopped and joined on destruction: a failed fatal assertion returns from the test, the thread must not outlive it.
 */
class PowerThread
{
public:
  PowerThread() :
    _thread([this]() {
      while (not _shouldStop)
      {
        // counted before the call: a change means that the thread is entering loop()
        _loopCount.fetch_add(1);
        powergates::loop();
      }
    })
  {
  }

  ~PowerThread() { stop(); }

  PowerThread(const PowerThread&) = delete;
  PowerThread& operator=(const PowerThread&) = delete;

  void stop()
  {
    _shouldStop = true;
    if (_thread.joinable())
      _thread.join();
  }

  /// Wait until the thread enters a new loop() call
  void wait_for_next_loop() const
  {
    const uint32_t loopCount = _loopCount.load();
    while (_loopCount.load() == loopCount)
    {
    }
  }

private:
  std::atomic<bool> _shouldStop {false};
  std::atomic<uint32_t> _loopCount {0};
  // last: started once the other members are initialized
  std::thread _thread;
};

} // namespace

/// Power gates started with a normal clock, and the output gate disabled
class power_gates_test : public ::testing::Test
{
protected:
  /// clock value at the start of the test
  virtual uint32_t start_clock_ms() const { return 0; }

  void SetUp() override
  {
    ::simulator::time_mocks::reset(start_clock_ms());
    powergates::init();
  }

  void TearDown() override
  {
    powergates::disable_gates();
    ::simulator::time_mocks::reset();
  }
};

/// Power gates started with the output gate on
class power_gates_blip_test : public power_gates_test
{
protected:
  void SetUp() override
  {
    power_gates_test::SetUp();
    enable_power_gate_and_wait();
    ASSERT_TRUE(powergates::is_power_gate_enabled());
    ASSERT_TRUE(is_output_gate_pin_high());
    ASSERT_FALSE(is_vbus_gate_pin_high());
    ASSERT_FALSE(power::is_bliping());
  }
};

/// Power gates started with the output gate on, just before the wrap of the 32 bits milliseconds clock
class power_gates_blip_wrap_test : public power_gates_blip_test
{
protected:
  uint32_t start_clock_ms() const override { return UINT32_MAX - timeBeforeWrap_ms + 1; }
};

// ============ blip refused ============

/// No blip without the output gate on: nothing to cut
TEST_F(power_gates_test, blip_refused_when_power_gate_disabled)
{
  ASSERT_FALSE(powergates::is_power_gate_enabled());

  power::blip(blipDuration_ms);
  EXPECT_FALSE(power::is_bliping());
  EXPECT_FALSE(is_output_gate_pin_high());

  // a cancel must not enable the output gate
  power::cancel_blip();
  powergates::loop();
  EXPECT_FALSE(is_output_gate_pin_high());
}

/// No blip on the vbus gate: the output gate must stay off
TEST_F(power_gates_test, blip_refused_when_vbus_gate_enabled)
{
  powergates::enable_vbus_gate_DIRECT();
  ASSERT_TRUE(is_vbus_gate_pin_high());

  power::blip(blipDuration_ms);
  EXPECT_FALSE(power::is_bliping());

  power::cancel_blip();
  run_loop_for(20);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(is_vbus_gate_pin_high());
}

/// No blip while the output gate is still switching on
TEST_F(power_gates_test, blip_refused_while_power_gate_switching)
{
  powergates::enable_power_gate();
  ASSERT_FALSE(powergates::is_power_gate_enabled());

  power::blip(blipDuration_ms);
  EXPECT_FALSE(power::is_bliping());

  // the switch still ends normally
  const uint32_t start_ms = hal::time_ms();
  wait_elapsed(start_ms, gateSwitchWait_ms);
  powergates::loop();
  EXPECT_TRUE(powergates::is_power_gate_enabled());
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
}

/// Empty and too long blips are refused, the output stays on
TEST_F(power_gates_blip_test, invalid_durations_refused)
{
  for (const uint32_t duration_ms: {0u, blipMaxDuration_ms + 1, UINT32_MAX})
  {
    power::blip(duration_ms);
    EXPECT_FALSE(power::is_bliping()) << "duration " << duration_ms;
    EXPECT_TRUE(is_output_gate_pin_high()) << "duration " << duration_ms;
  }
}

/// A refused blip during a blip ends it: the output gate must not stay off with no blip to restore it
TEST_F(power_gates_blip_test, invalid_blip_during_blip_restores_output)
{
  for (const uint32_t duration_ms: {0u, blipMaxDuration_ms + 1, UINT32_MAX})
  {
    power::blip(blipDuration_ms);
    ASSERT_FALSE(is_output_gate_pin_high());

    power::blip(duration_ms);
    EXPECT_FALSE(power::is_bliping()) << "duration " << duration_ms;
    EXPECT_TRUE(is_output_gate_pin_high()) << "duration " << duration_ms;

    run_loop_for(20);
    EXPECT_TRUE(is_output_gate_pin_high()) << "duration " << duration_ms;
    EXPECT_FALSE(is_vbus_gate_pin_high()) << "duration " << duration_ms;
  }
}

// ============ blip timing ============

/// The output gate is off for the blip duration, then restored by the loop
TEST_F(power_gates_blip_test, blip_cuts_then_restores_output)
{
  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(blipDuration_ms);

  // cut at once, not on the next loop
  EXPECT_TRUE(power::is_bliping());
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_FALSE(is_vbus_gate_pin_high());

  // off for the whole duration
  while (elapsed_since(blipStart_ms) < blipDuration_ms - 50)
  {
    powergates::loop();
    ASSERT_FALSE(is_output_gate_pin_high()) << "restored after " << elapsed_since(blipStart_ms) << "ms";
    ASSERT_TRUE(power::is_bliping());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  wait_elapsed(blipStart_ms, blipDuration_ms);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
  EXPECT_FALSE(is_vbus_gate_pin_high());
  EXPECT_TRUE(powergates::is_power_gate_enabled());
}

/// Without a loop call, the output stays off: the restoration is done by the loop
TEST_F(power_gates_blip_test, blip_restored_only_by_loop)
{
  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(50);

  wait_elapsed(blipStart_ms, 100);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
}

/// The longest accepted blip is accepted
TEST_F(power_gates_blip_test, max_duration_accepted)
{
  power::blip(blipMaxDuration_ms);
  EXPECT_TRUE(power::is_bliping());
  EXPECT_FALSE(is_output_gate_pin_high());

  run_loop_for(20);
  EXPECT_TRUE(power::is_bliping());
  EXPECT_FALSE(is_output_gate_pin_high());

  power::cancel_blip();
  EXPECT_TRUE(is_output_gate_pin_high());
}

/// A new blip during a blip restarts the timing from the new request
TEST_F(power_gates_blip_test, new_blip_restarts_timing)
{
  const uint32_t firstStart_ms = hal::time_ms();
  power::blip(blipDuration_ms);

  wait_elapsed(firstStart_ms, blipDuration_ms - 100);
  const uint32_t secondStart_ms = hal::time_ms();
  power::blip(blipDuration_ms);

  // past the end of the first blip: still off
  wait_elapsed(firstStart_ms, blipDuration_ms + 100);
  ASSERT_LT(elapsed_since(secondStart_ms), blipDuration_ms) << "the test was too slow";
  powergates::loop();
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  // end of the second blip
  wait_elapsed(secondStart_ms, blipDuration_ms);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
}

/// Successive blips each cut and restore the output
TEST_F(power_gates_blip_test, successive_blips)
{
  for (int i = 0; i < 5; ++i)
  {
    const uint32_t blipStart_ms = hal::time_ms();
    power::blip(30);
    EXPECT_FALSE(is_output_gate_pin_high()) << "blip " << i;
    EXPECT_TRUE(power::is_bliping()) << "blip " << i;

    wait_elapsed(blipStart_ms, 30);
    powergates::loop();
    EXPECT_TRUE(is_output_gate_pin_high()) << "blip " << i;
    EXPECT_FALSE(power::is_bliping()) << "blip " << i;
  }
}

// ============ blip cancellation ============

/// A cancel restores the output at once
TEST_F(power_gates_blip_test, cancel_restores_output)
{
  power::blip(blipDuration_ms);
  ASSERT_FALSE(is_output_gate_pin_high());

  power::cancel_blip();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());

  // the loop keeps it on, after the end of the cancelled blip too
  run_loop_for(20);
  EXPECT_TRUE(is_output_gate_pin_high());
}

/// A cancel without a blip changes nothing
TEST_F(power_gates_blip_test, cancel_without_blip)
{
  power::cancel_blip();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
  EXPECT_FALSE(is_vbus_gate_pin_high());
}

/// A blip after a cancel has its own full duration
TEST_F(power_gates_blip_test, blip_after_cancel)
{
  power::blip(blipDuration_ms);
  power::cancel_blip();

  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(100);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  run_loop_for(50);
  EXPECT_FALSE(is_output_gate_pin_high());

  wait_elapsed(blipStart_ms, 100);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
}

// ============ blip and gate switches ============

/// Disabling the output gate ends the blip: nothing may enable the output gate again
TEST_F(power_gates_blip_test, disable_power_gate_during_blip)
{
  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(100);

  powergates::disable_gates();
  EXPECT_FALSE(power::is_bliping());
  EXPECT_FALSE(is_output_gate_pin_high());

  power::cancel_blip();
  EXPECT_FALSE(is_output_gate_pin_high());

  // after the end of the blip
  wait_elapsed(blipStart_ms, 150);
  run_loop_for(20);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_FALSE(is_vbus_gate_pin_high());
}

/// Switching to the vbus gate ends the blip: both gates must never be on together
TEST_F(power_gates_blip_test, vbus_gate_during_blip)
{
  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(100);

  powergates::enable_vbus_gate_DIRECT();
  EXPECT_TRUE(is_vbus_gate_pin_high());
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_FALSE(powergates::is_power_gate_enabled());
  EXPECT_FALSE(power::is_bliping());

  power::cancel_blip();
  EXPECT_FALSE(is_output_gate_pin_high());

  // after the end of the blip
  wait_elapsed(blipStart_ms, 150);
  run_loop_for(20);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(is_vbus_gate_pin_high());
}

/// Delayed switch to the vbus gate during a blip: the output gate must not come back on
TEST_F(power_gates_blip_test, delayed_vbus_gate_during_blip)
{
  power::blip(blipDuration_ms);

  powergates::enable_vbus_gate();
  EXPECT_FALSE(is_output_gate_pin_high());

  run_loop_for(gateSwitchWait_ms + 20);
  EXPECT_TRUE(is_vbus_gate_pin_high());
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());

  power::cancel_blip();
  EXPECT_FALSE(is_output_gate_pin_high());
}

/// The output gate can be enabled again after a blip interrupted by a gate disable, and blip again
TEST_F(power_gates_blip_test, blip_after_power_gate_cycle)
{
  power::blip(blipDuration_ms);
  powergates::disable_gates();

  enable_power_gate_and_wait();
  ASSERT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());

  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(50);
  EXPECT_FALSE(is_output_gate_pin_high());
  wait_elapsed(blipStart_ms, 50);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
}

// ============ threads ============

/**
 * The loop runs on the power thread, the blips are requested from the user thread.
 * A blip started while the loop runs must never be erased, nor restored early.
 */
TEST_F(power_gates_blip_test, blip_from_other_thread_while_loop_runs)
{
  // stopped and joined when leaving the test, a failed assertion included
  PowerThread powerThread;

  for (int i = 0; i < 300; ++i)
  {
    // long blip: the loop can not expire it during the checks
    power::blip(blipDuration_ms);
    ASSERT_TRUE(power::is_bliping()) << "iteration " << i;
    ASSERT_FALSE(is_output_gate_pin_high()) << "iteration " << i;
    ASSERT_FALSE(is_vbus_gate_pin_high()) << "iteration " << i;

    power::cancel_blip();
    ASSERT_FALSE(power::is_bliping()) << "iteration " << i;
    ASSERT_TRUE(is_output_gate_pin_high()) << "iteration " << i;
  }

  // short blips, expired by the power thread
  for (int i = 0; i < 20; ++i)
  {
    const uint32_t blipStart_ms = hal::time_ms();
    power::blip(5);
    while (power::is_bliping())
    {
      ASSERT_LT(elapsed_since(blipStart_ms), 200u) << "the blip never ended, iteration " << i;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_GE(elapsed_since(blipStart_ms), 5u) << "iteration " << i;
    EXPECT_TRUE(is_output_gate_pin_high()) << "iteration " << i;
  }

  powerThread.stop();
}

/**
 * A replacement blip started while the power thread expires the previous one must last its full duration.
 * The replacement is sent when the previous blip times out, as the power thread enters the loop() call that handles
 * the timeout: the timeout handling must not erase the new blip.
 */
TEST_F(power_gates_blip_test, replacement_blip_during_timeout_handling)
{
  static constexpr uint32_t shortBlip_ms = 5;
  static constexpr uint32_t replacementBlip_ms = 60;
  // the scheduler can delay the checks: only check the blip until this margin before its end
  static constexpr uint32_t checkMargin_ms = 15;

  // stopped and joined when leaving the test, a failed assertion included
  PowerThread powerThread;

  for (int i = 0; i < 15; ++i)
  {
    const uint32_t blipStart_ms = hal::time_ms();
    power::blip(shortBlip_ms);

    // the previous blip times out now: send the replacement as the power thread handles it
    while (elapsed_since(blipStart_ms) < shortBlip_ms)
    {
    }
    powerThread.wait_for_next_loop();
    const uint32_t replacementStart_ms = hal::time_ms();
    power::blip(replacementBlip_ms);

    // the replacement is active for its whole duration
    while (elapsed_since(replacementStart_ms) < replacementBlip_ms - checkMargin_ms)
    {
      ASSERT_TRUE(power::is_bliping())
              << "replacement cancelled after " << elapsed_since(replacementStart_ms) << "ms, iteration " << i;
      ASSERT_FALSE(is_output_gate_pin_high())
              << "output restored after " << elapsed_since(replacementStart_ms) << "ms, iteration " << i;
      ASSERT_FALSE(is_vbus_gate_pin_high()) << "iteration " << i;
    }

    // then ends, and restores the output
    while (power::is_bliping())
    {
      ASSERT_LT(elapsed_since(replacementStart_ms), replacementBlip_ms + 200) << "the blip never ended, iteration " << i;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_GE(elapsed_since(replacementStart_ms), replacementBlip_ms) << "iteration " << i;
    EXPECT_TRUE(is_output_gate_pin_high()) << "iteration " << i;
  }

  powerThread.stop();
}

// ============ clock wrap ============

/// A blip started before the clock wrap must last its full duration, then restore the output gate
TEST_F(power_gates_blip_wrap_test, blip_across_clock_wrap)
{
  // the blip starts before the wrap, and ends after it
  const uint32_t blipStart_ms = hal::time_ms();
  ASSERT_GT(blipStart_ms, UINT32_MAX - timeBeforeWrap_ms) << "the clock already wrapped";
  ASSERT_LT(UINT32_MAX - blipStart_ms, blipDuration_ms) << "the blip would end before the wrap";
  power::blip(blipDuration_ms);

  // before the wrap: the gate is off
  powergates::loop();
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  // wait for the wrap
  while (hal::time_ms() >= blipStart_ms)
  {
    powergates::loop();
    ASSERT_FALSE(is_output_gate_pin_high()) << "the blip ended before the wrap";
    ASSERT_TRUE(power::is_bliping());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  ASSERT_LT(elapsed_since(blipStart_ms), blipDuration_ms) << "the test was too slow to see the blip after the wrap";

  // after the wrap: the clock value is small, but the blip is not over
  powergates::loop();
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  // the blip is over: the output gate is restored
  wait_elapsed(blipStart_ms, blipDuration_ms);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
  EXPECT_TRUE(powergates::is_power_gate_enabled());
}

/// A blip started just after the wrap must not be expired by the clock jump
TEST_F(power_gates_blip_wrap_test, blip_started_after_clock_wrap)
{
  while (hal::time_ms() > UINT32_MAX - timeBeforeWrap_ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

  const uint32_t blipStart_ms = hal::time_ms();
  power::blip(100);

  run_loop_for(50);
  EXPECT_FALSE(is_output_gate_pin_high());
  EXPECT_TRUE(power::is_bliping());

  wait_elapsed(blipStart_ms, 100);
  powergates::loop();
  EXPECT_TRUE(is_output_gate_pin_high());
  EXPECT_FALSE(power::is_bliping());
}

} // namespace lampda
