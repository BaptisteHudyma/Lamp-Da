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

  s.vbusVoltage_mV = 20000 - policy::pdVoltageMargin_mV - 1;
  expect_no_current(s);

  s.vbusVoltage_mV = 20000 - policy::pdVoltageMargin_mV;
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
