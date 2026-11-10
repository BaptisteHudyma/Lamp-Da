#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "src/system/bsp/pd/input_power_policy.h"
#include "src/system/bsp/pd/power_delivery.h"
#include "src/system/bsp/pd/usb_pd.h"
#include "src/system/driver/pd/usb_pd_driver.h"

#include "simulator/include/hardware_influencer.h"

namespace lampda {

namespace policy = bsp::powerDelivery::policy;

namespace {

/// A plain 5V source, detected since the given time, with no PD activity
policy::SourceStatus standard_source(const uint32_t timeSinceDetection_ms)
{
  policy::SourceStatus s;
  s.isPowerSourceDetected = true;
  s.timeSinceSourceDetected_ms = timeSinceDetection_ms;
  s.timeSinceLastPdDetected_ms = timeSinceDetection_ms + 10000;
  s.vbusVoltage_mV = 5000;
  return s;
}

/// A PD source with an established contract, VBUS at the negociated voltage
policy::SourceStatus pd_contract(const uint32_t voltage_mV, const uint32_t current_mA)
{
  policy::SourceStatus s;
  s.isPowerSourceDetected = true;
  s.timeSinceSourceDetected_ms = 3000;
  s.timeSinceLastPdDetected_ms = 0;
  s.isUsbPd = true;
  s.isSinkReady = true;
  s.negotiatedVoltage_mV = voltage_mV;
  s.negotiatedCurrent_mA = current_mA;
  s.vbusVoltage_mV = voltage_mV;
  return s;
}

void expect_no_current(const policy::SourceStatus& s)
{
  EXPECT_FALSE(policy::can_use_power(s));
  EXPECT_EQ(policy::max_input_current_mA(s), 0);
}

} // namespace

//
// Input power rules
//

TEST(charging_policy, no_source_no_current)
{
  policy::SourceStatus s;
  expect_no_current(s);

  // stale values of a previous source must not be used
  s = pd_contract(20000, 3000);
  s.isPowerSourceDetected = false;
  expect_no_current(s);
}

TEST(charging_policy, standard_port_waits_for_the_detection_delay)
{
  expect_no_current(standard_source(0));
  expect_no_current(standard_source(policy::standardPortDetectionDelay_ms));

  const auto s = standard_source(policy::standardPortDetectionDelay_ms + 1);
  EXPECT_TRUE(policy::is_standard_port(s));
  EXPECT_TRUE(policy::can_use_power(s));
  EXPECT_EQ(policy::max_input_current_mA(s), policy::standardPortCurrent_mA);
}

TEST(charging_policy, standard_port_blocked_after_recent_pd_activity)
{
  auto s = standard_source(3000);
  s.timeSinceLastPdDetected_ms = policy::pdLostDelay_ms;
  expect_no_current(s);

  s.timeSinceLastPdDetected_ms = policy::pdLostDelay_ms + 1;
  EXPECT_EQ(policy::max_input_current_mA(s), policy::standardPortCurrent_mA);
}

/// Issue #633: a late PD negociation (missed source caps, soft then hard reset) must not see a standard port
TEST(charging_policy, standard_port_waits_for_a_late_pd_negociation)
{
  auto s = standard_source(2500);
  s.isSinkNegotiating = true;
  expect_no_current(s);
  EXPECT_FALSE(policy::is_standard_port(s));

  // the negociation failed: non PD source
  s.isSinkNegotiating = false;
  EXPECT_EQ(policy::max_input_current_mA(s), policy::standardPortCurrent_mA);
}

TEST(charging_policy, standard_port_does_not_wait_forever_for_pd)
{
  auto s = standard_source(policy::maxPdNegociationWait_ms);
  s.isSinkNegotiating = true;
  expect_no_current(s);

  s.timeSinceSourceDetected_ms = policy::maxPdNegociationWait_ms + 1;
  EXPECT_TRUE(policy::can_use_power(s));
  EXPECT_EQ(policy::max_input_current_mA(s), policy::standardPortCurrent_mA);
}

/// When the PD state machine resumes, the detection time restarts: no current until the delay passed again
TEST(charging_policy, pd_resume_restarts_the_standard_port_delay)
{
  // the source was plugged long ago, while PD was suspended (lamp on)
  auto s = standard_source(60000);
  EXPECT_TRUE(policy::can_use_power(s));

  // lamp turned off: PD resumes, the detection time is restarted
  s.timeSinceSourceDetected_ms = 0;
  expect_no_current(s);
  // PD attach in progress
  s.timeSinceSourceDetected_ms = 200;
  s.isSinkNegotiating = true;
  expect_no_current(s);
}

TEST(charging_policy, pd_source_caps_without_contract_give_no_current)
{
  auto s = pd_contract(20000, 3000);
  s.isSinkReady = false;
  s.isSinkNegotiating = true;
  expect_no_current(s);

  // no contract published yet
  s.isSinkReady = true;
  s.negotiatedVoltage_mV = 0;
  s.negotiatedCurrent_mA = 0;
  expect_no_current(s);
}

TEST(charging_policy, pd_contract_waits_for_vbus_to_rise)
{
  auto s = pd_contract(20000, 3000);
  // still at 5V, the source is transitioning
  s.vbusVoltage_mV = 5000;
  expect_no_current(s);

  s.vbusVoltage_mV = policy::pd_min_established_vbus_mV(20000) - 1;
  expect_no_current(s);

  s.vbusVoltage_mV = policy::pd_min_established_vbus_mV(20000);
  EXPECT_TRUE(policy::can_use_power(s));
}

TEST(charging_policy, pd_contract_accepts_the_vbus_tolerances)
{
  // 9V from another lamp: the FUSB302 read 7.98V (420mV steps, rounded down), with no current yet (issue #357)
  auto s = pd_contract(9000, 3000);
  s.vbusVoltage_mV = 7980;
  EXPECT_TRUE(policy::can_use_power(s));

  // the source is still at 5V
  s.vbusVoltage_mV = 5040;
  expect_no_current(s);

  // a 5V contract
  s = pd_contract(5000, 3000);
  s.vbusVoltage_mV = 4200;
  EXPECT_TRUE(policy::can_use_power(s));
}

TEST(charging_policy, pd_contract_uses_part_of_the_negociated_current)
{
  const auto s = pd_contract(20000, 3000);
  EXPECT_TRUE(policy::can_use_pd_full_power(s));
  EXPECT_TRUE(policy::can_use_power(s));
  EXPECT_EQ(policy::max_input_current_mA(s), 2700);

  EXPECT_EQ(policy::max_input_current_mA(pd_contract(9000, 2000)), 1800);
}

TEST(charging_policy, pd_source_never_seen_as_standard_port)
{
  // a PD source without contract must not fall back to the standard current, even after a long time
  auto s = pd_contract(20000, 3000);
  s.isSinkReady = false;
  s.timeSinceSourceDetected_ms = 60000;
  expect_no_current(s);
}

TEST(charging_policy, hard_reset_during_contract_cuts_current)
{
  auto s = pd_contract(20000, 3000);
  ASSERT_GT(policy::max_input_current_mA(s), 0);

  // hard reset: the contract is lost, VBUS goes down
  s.isSinkReady = false;
  s.isSinkNegotiating = true;
  expect_no_current(s);
  s.vbusVoltage_mV = 0;
  expect_no_current(s);
}

TEST(charging_policy, unplug_detected_after_the_deglitch)
{
  EXPECT_FALSE(policy::is_unplugged(0, false));
  EXPECT_FALSE(policy::is_unplugged(policy::unplugDeglitch_ms, false));
  EXPECT_TRUE(policy::is_unplugged(policy::unplugDeglitch_ms + 1, false));
}

/// A source hard reset drops VBUS for up to ~2s: this is not an unplug
TEST(charging_policy, hard_reset_vbus_drop_is_not_an_unplug)
{
  static constexpr uint32_t sourceHardResetVbusOff_ms = 650 + 1000 + 275;
  EXPECT_FALSE(policy::is_unplugged(sourceHardResetVbusOff_ms, true));
  EXPECT_FALSE(policy::is_unplugged(policy::hardResetUnplugDeglitch_ms, true));
}

/// Issue #642: a PD state machine stuck in a hard reset state must not keep an unplugged source detected forever
TEST(charging_policy, unplug_detected_even_if_stuck_in_hard_reset)
{
  EXPECT_TRUE(policy::is_unplugged(policy::hardResetUnplugDeglitch_ms + 1, true));
  EXPECT_TRUE(policy::is_unplugged(60 * 1000, true));
}

/// A VBUS loss without hard reset uses the normal deglitch
TEST(charging_vbus_loss, loss_without_hard_reset_uses_the_normal_deglitch)
{
  policy::VbusLossTracker tracker;
  tracker.on_vbus_valid(10000);

  EXPECT_FALSE(tracker.is_unplugged(10000 + policy::unplugDeglitch_ms, false));
  EXPECT_TRUE(tracker.is_unplugged(10000 + policy::unplugDeglitch_ms + 1, false));
}

/// The state machine leaves the hard reset states before VBUS is back over our threshold: the hard reset grace stays
TEST(charging_vbus_loss, hard_reset_grace_survives_a_pd_state_change)
{
  policy::VbusLossTracker tracker;
  tracker.on_vbus_valid(10000);

  // hard reset: the source drops VBUS
  EXPECT_FALSE(tracker.is_unplugged(10100, true));
  // the state machine left the hard reset (discovery), VBUS still under the threshold: not an unplug
  EXPECT_FALSE(tracker.is_unplugged(10000 + policy::unplugDeglitch_ms + 1, false));
  EXPECT_FALSE(tracker.is_unplugged(10000 + policy::hardResetUnplugDeglitch_ms, false));

  // VBUS is back: the next loss starts without the hard reset grace
  tracker.on_vbus_valid(12000);
  EXPECT_TRUE(tracker.is_unplugged(12000 + policy::unplugDeglitch_ms + 1, false));
}

/// The grace deadline is counted from the original VBUS loss: no PD state change can extend it (issue #642)
TEST(charging_vbus_loss, hard_reset_grace_bounded_by_the_vbus_loss_time)
{
  policy::VbusLossTracker tracker;
  tracker.on_vbus_valid(10000);

  // a hard reset seen late in the loss, then the state machine goes back and forth
  EXPECT_FALSE(tracker.is_unplugged(10900, true));
  EXPECT_FALSE(tracker.is_unplugged(11500, false));
  EXPECT_FALSE(tracker.is_unplugged(12500, true));
  EXPECT_FALSE(tracker.is_unplugged(10000 + policy::hardResetUnplugDeglitch_ms, true));

  // the deadline does not move, whatever the state
  EXPECT_TRUE(tracker.is_unplugged(10000 + policy::hardResetUnplugDeglitch_ms + 1, true));
  EXPECT_TRUE(tracker.is_unplugged(10000 + policy::hardResetUnplugDeglitch_ms + 1, false));
  // stuck in a hard reset state forever: still an unplug
  EXPECT_TRUE(tracker.is_unplugged(10000 + 60 * 1000, true));
}

/// The time wraps after ~49 days: the loss duration is still right
TEST(charging_vbus_loss, time_wrap_around)
{
  policy::VbusLossTracker tracker;
  tracker.on_vbus_valid(UINT32_MAX - 500);

  EXPECT_FALSE(tracker.is_unplugged(UINT32_MAX - 400, true));
  EXPECT_FALSE(tracker.is_unplugged(policy::hardResetUnplugDeglitch_ms - 501, false));
  EXPECT_TRUE(tracker.is_unplugged(policy::hardResetUnplugDeglitch_ms - 499, false));
}

//
// PD task scheduler
//

TEST(charging_pd_task, no_timeout_is_an_infinite_wait) { EXPECT_EQ(task_timeout_us_to_ms(0), 0u); }

/// Issue #642: a sub millisecond timeout was truncated to 0, an infinite wait: the PD state machine missed its state
/// timeout (stuck in hard reset recovery after an unplug), until an unrelated interrupt woke it up
TEST(charging_pd_task, sub_millisecond_timeout_is_not_an_infinite_wait)
{
  EXPECT_EQ(task_timeout_us_to_ms(1), 1u);
  EXPECT_EQ(task_timeout_us_to_ms(400), 1u);
  EXPECT_EQ(task_timeout_us_to_ms(999), 1u);
}

TEST(charging_pd_task, timeout_rounded_up_to_the_millisecond)
{
  EXPECT_EQ(task_timeout_us_to_ms(1000), 1u);
  EXPECT_EQ(task_timeout_us_to_ms(1001), 2u);
  EXPECT_EQ(task_timeout_us_to_ms(500 * 1000), 500u);
  EXPECT_EQ(task_timeout_us_to_ms(1275 * 1000 + 1), 1276u);
}

//
// PD driver
//

class charging_pd_driver : public ::testing::Test
{
protected:
  void SetUp() override { reset_cache(); }
  void TearDown() override { reset_cache(); }

  uint32_t caps[2] = {PDO_FIXED(5000, 3000, 0), PDO_FIXED(20000, 3000, 0)};
};

/// Issue #633: the source caps are an offer, not a contract: the input current limit must not change
TEST_F(charging_pd_driver, source_caps_do_not_publish_an_input_limit)
{
  pd_process_source_cap(2, caps);

  EXPECT_EQ(get_available_pd_current_mA(), 0u);
  EXPECT_EQ(get_available_pd_voltage_mV(), 0u);
}

/// A new source cap on an established contract must keep the contract limits
TEST_F(charging_pd_driver, source_caps_keep_the_current_contract)
{
  pd_set_input_current_limit(1500, 9000);

  pd_process_source_cap(2, caps);

  EXPECT_EQ(get_available_pd_current_mA(), 1500u);
  EXPECT_EQ(get_available_pd_voltage_mV(), 9000u);
}

TEST_F(charging_pd_driver, contract_published_and_cleared)
{
  pd_set_input_current_limit(3000, 20000);
  EXPECT_EQ(get_available_pd_current_mA(), 3000u);
  EXPECT_EQ(get_available_pd_voltage_mV(), 20000u);

  reset_cache();
  EXPECT_EQ(get_available_pd_current_mA(), 0u);
  EXPECT_EQ(get_available_pd_voltage_mV(), 0u);
}

TEST_F(charging_pd_driver, reset_source_capabilities_forgets_the_pd_source)
{
  pd_process_source_cap_callback(2, caps);
  EXPECT_TRUE(is_pd_conector());
  EXPECT_EQ(get_pd_source_cnt(), 2);

  reset_source_capabilities();
  EXPECT_FALSE(is_pd_conector());
  EXPECT_EQ(get_pd_source_cnt(), 0);
}

/// Sources can send up to 7 PDOs (PPS chargers often do): only the first ones are kept, nothing is written past them
TEST_F(charging_pd_driver, source_caps_with_seven_pdos_do_not_overflow)
{
  uint32_t sevenCaps[7];
  for (uint32_t i = 0; i < 7; ++i)
    sevenCaps[i] = PDO_FIXED(5000 + i * 1000, 3000, 0);

  // with ASan, a write past the saved capabilities fails here
  pd_process_source_cap_callback(7, sevenCaps);

  EXPECT_TRUE(is_pd_conector());
  EXPECT_EQ(get_pd_source_cnt(), 5);
  for (uint8_t i = 0; i < 5; ++i)
    EXPECT_EQ(get_pd_source(i), sevenCaps[i]);
  // the contract limits are not touched
  EXPECT_EQ(get_available_pd_current_mA(), 0u);
  EXPECT_EQ(get_available_pd_voltage_mV(), 0u);
}

TEST_F(charging_pd_driver, get_pd_source_out_of_range_is_zero)
{
  pd_process_source_cap_callback(2, caps);
  EXPECT_EQ(get_pd_source(0), caps[0]);
  EXPECT_EQ(get_pd_source(1), caps[1]);
  EXPECT_EQ(get_pd_source(2), 0u);
  EXPECT_EQ(get_pd_source(5), 0u);
}

/// With the source caps stored, a PD contract gets the PD current, not the standard port fallback (issue #633)
TEST_F(charging_pd_driver, pd_contract_uses_pd_current_not_standard_port)
{
  pd_process_source_cap_callback(2, caps);
  pd_set_input_current_limit(1670, 9000);

  policy::SourceStatus s;
  s.isPowerSourceDetected = true;
  s.timeSinceSourceDetected_ms = 10000;
  s.timeSinceLastPdDetected_ms = 0;
  s.isUsbPd = is_pd_conector() != 0;
  s.isSinkReady = true;
  s.negotiatedVoltage_mV = get_available_pd_voltage_mV();
  s.negotiatedCurrent_mA = get_available_pd_current_mA();
  s.vbusVoltage_mV = 8700;

  EXPECT_FALSE(policy::is_standard_port(s));
  EXPECT_EQ(policy::max_input_current_mA(s), static_cast<uint16_t>(1670 * policy::pdCurrentRatio));
}

/// Recovery mode: a source reset would brown out the board, so the resets are forbidden until the system allows them
TEST_F(charging_pd_driver, source_resets_forbidden_until_allowed)
{
  EXPECT_FALSE(pd_is_reset_allowed());

  bsp::powerDelivery::allow_source_resets(true);
  EXPECT_TRUE(pd_is_reset_allowed());
  // a state machine that is not waiting for source caps is not affected
  EXPECT_FALSE(is_sink_negotiating());

  bsp::powerDelivery::allow_source_resets(false);
  EXPECT_FALSE(pd_is_reset_allowed());
}

TEST_F(charging_pd_driver, stopped_state_machine_is_not_negotiating)
{
  // never started in the tests
  EXPECT_FALSE(is_sink_negotiating());
  EXPECT_FALSE(is_sink_ready());
}

//
// Power delivery component, with the simulated VBUS
//

namespace {

/// Run the power delivery loop for some time
void run_pd_loop_for(const std::chrono::milliseconds duration)
{
  const auto end = std::chrono::steady_clock::now() + duration;
  while (std::chrono::steady_clock::now() < end)
  {
    bsp::powerDelivery::loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

} // namespace

/// A plain 5V source is only used after the detection delay, and stops being used when unplugged
TEST(charging_power_delivery, standard_source_used_after_detection_delay)
{
  using namespace std::chrono_literals;
  auto& vbus = ::simulator::mock_electrical::inputVbusVoltage;
  const float previousVbus = vbus;

  // the source is unplugged (and the unplug deglitched)
  vbus = 0.0f;
  run_pd_loop_for(1200ms);
  ASSERT_FALSE(bsp::powerDelivery::is_power_available());

  // plug a standard 5V source
  vbus = 5.0f;
  run_pd_loop_for(300ms);
  EXPECT_TRUE(bsp::powerDelivery::is_power_available());
  // too early: the source type is not known yet
  EXPECT_FALSE(bsp::powerDelivery::can_use_power());
  EXPECT_EQ(bsp::powerDelivery::get_max_input_current(), 0);

  run_pd_loop_for(std::chrono::milliseconds(policy::standardPortDetectionDelay_ms));
  EXPECT_TRUE(bsp::powerDelivery::is_standard_port());
  EXPECT_TRUE(bsp::powerDelivery::can_use_power());
  EXPECT_EQ(bsp::powerDelivery::get_max_input_current(), policy::standardPortCurrent_mA);

  // unplug: the current use stops when vbus is lost
  vbus = 0.0f;
  run_pd_loop_for(1200ms);
  EXPECT_FALSE(bsp::powerDelivery::is_power_available());
  EXPECT_FALSE(bsp::powerDelivery::can_use_power());
  EXPECT_EQ(bsp::powerDelivery::get_max_input_current(), 0);

  vbus = previousVbus;
}

} // namespace lampda
