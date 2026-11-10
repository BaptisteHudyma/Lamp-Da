#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

#include "src/system/component/battery.h"
#include "src/system/component/charge_policy.h"
#include "src/system/utils/constants.h"

namespace lampda {

namespace policy = component::charger::policy;

namespace {

/// Charging, with balanced cells at the given voltage
policy::Inputs charging_at(const uint16_t cell_mV, const uint16_t chargeCurrent_mA)
{
  policy::Inputs in;
  in.isBalancerValid = true;
  in.maxCell_mV = cell_mV;
  in.pack_mV = cell_mV * physical_constants::batterySeriesCount;
  in.level = component::battery::get_level_safe(in.pack_mV);
  in.chargeCurrent_mA = chargeCurrent_mA;
  in.isCharging = true;
  return in;
}

/// Charging, with the given cell voltages
policy::Inputs charging_cells(const uint16_t c0, const uint16_t c1, const uint16_t c2, const uint16_t chargeCurrent_mA)
{
  static_assert(physical_constants::batterySeriesCount == 3, "test written for 3 cells");
  policy::Inputs in = charging_at(c0, chargeCurrent_mA);
  in.maxCell_mV = std::max<uint16_t>({c0, c1, c2});
  in.pack_mV = c0 + c1 + c2;
  in.level = component::battery::get_level_safe(in.pack_mV);
  return in;
}

/// Run the limiter with the same inputs, every 100ms, for some time. Return the last result
bool run_for(policy::ChargeLimiter& limiter, uint32_t& time_ms, const policy::Inputs& in, const uint32_t duration_ms)
{
  bool isAllowed = limiter.update(time_ms, in);
  for (uint32_t elapsed = 0; elapsed < duration_ms; elapsed += 100)
  {
    time_ms += 100;
    isAllowed = limiter.update(time_ms, in);
  }
  return isAllowed;
}

} // namespace

TEST(charge_policy, charge_allowed_below_the_limits)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  EXPECT_TRUE(run_for(limiter, time, charging_at(3800, 1600), 60000));
  EXPECT_FALSE(limiter.is_full());
  EXPECT_FALSE(limiter.is_over_voltage_stopped());
}

/// Measured on a lamp with a PD charger: cells 4113/4106/4110mV (12329mV), the charge never stopped
TEST(charge_policy, pack_over_the_target_ends_the_charge)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  const auto in = charging_cells(4113, 4106, 4110, 192);

  EXPECT_FALSE(limiter.update(time, in));
  EXPECT_TRUE(limiter.is_over_voltage_stopped());
  EXPECT_TRUE(limiter.is_full());
}

/// Measured on a lamp: cells 4112/4103/4109mV (12324mV), 192mA of charge current while the balancer bleeds
TEST(charge_policy, end_of_charge_with_the_balancer_bleeding)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  const auto in = charging_cells(4109, 4098, 4100, 192);
  ASSERT_FALSE(policy::is_over_voltage(in));

  // pack at the target: end of charge after the deglitch
  EXPECT_TRUE(limiter.update(time, in));
  EXPECT_FALSE(run_for(limiter, time, in, policy::endOfChargeDeglitch_ms + 200));
  EXPECT_TRUE(limiter.is_full());
  EXPECT_FALSE(limiter.is_over_voltage_stopped());
}

TEST(charge_policy, end_of_charge_on_current_taper)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  // pack just under the target (charger regulating low), current tapered off
  auto in = charging_at(4097, physical_constants::batteryEndOfChargeCurrent_mA);
  ASSERT_GE(in.level, policy::fullLevel);
  ASSERT_LT(in.pack_mV, physical_constants::batteryMaxVoltageSafe_mV);

  EXPECT_FALSE(run_for(limiter, time, in, policy::endOfChargeDeglitch_ms + 200));
  EXPECT_TRUE(limiter.is_full());
}

TEST(charge_policy, end_of_charge_needs_the_deglitch)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  const auto full = charging_at(4100, 100);

  EXPECT_TRUE(run_for(limiter, time, full, policy::endOfChargeDeglitch_ms - 200));
  // the condition disappears: the deglitch restarts
  EXPECT_TRUE(limiter.update(time += 100, charging_at(4050, 800)));
  EXPECT_TRUE(run_for(limiter, time, full, policy::endOfChargeDeglitch_ms - 200));
  EXPECT_FALSE(limiter.is_full());
}

/// A weak source gives a low charge current: this is not an end of charge
TEST(charge_policy, low_current_far_from_full_is_not_end_of_charge)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  EXPECT_TRUE(run_for(limiter, time, charging_at(3900, 64), 60000));
  EXPECT_FALSE(limiter.is_full());
}

TEST(charge_policy, not_charging_is_not_end_of_charge)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  auto in = charging_at(4100, 0);
  in.isCharging = false;
  EXPECT_TRUE(run_for(limiter, time, in, 60000));
  EXPECT_FALSE(limiter.is_full());
}

TEST(charge_policy, recharge_below_the_recharge_level)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  ASSERT_FALSE(run_for(limiter, time, charging_at(4100, 100), policy::endOfChargeDeglitch_ms + 200));
  ASSERT_TRUE(limiter.is_full());

  // the battery relaxes, still above the recharge level
  auto in = charging_at(4080, 0);
  in.isCharging = false;
  ASSERT_GE(in.level, policy::rechargeLevel);
  EXPECT_FALSE(run_for(limiter, time, in, 60000));

  // below the recharge level
  in = charging_at(4000, 0);
  in.isCharging = false;
  ASSERT_LT(in.level, policy::rechargeLevel);
  EXPECT_TRUE(limiter.update(time += 100, in));
  EXPECT_FALSE(limiter.is_full());
}

/// An unbalanced cell over the limit stops the charge before the pack is full, until the balancer brings it down
TEST(charge_policy, cell_over_voltage_pauses_the_charge)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  const auto unbalanced = charging_cells(policy::cellOverVoltage_mV + 1, 3950, 3950, 1000);
  ASSERT_LT(unbalanced.level, policy::fullLevel);

  EXPECT_FALSE(limiter.update(time, unbalanced));
  EXPECT_TRUE(limiter.is_over_voltage_stopped());
  EXPECT_FALSE(limiter.is_full());

  // the cell goes down, but not under the resume voltage
  const auto stillHigh = charging_cells(policy::cellResumeVoltage_mV, 3950, 3950, 0);
  EXPECT_FALSE(run_for(limiter, time, stillHigh, policy::overVoltagePause_ms * 2));

  // under the resume voltage, but the minimum pause is not over
  limiter = policy::ChargeLimiter();
  time = 1000;
  EXPECT_FALSE(limiter.update(time, unbalanced));
  const auto balanced = charging_cells(policy::cellResumeVoltage_mV - 1, 3950, 3950, 0);
  EXPECT_FALSE(run_for(limiter, time, balanced, policy::overVoltagePause_ms - 200));
  // pause over: the charge restarts
  EXPECT_TRUE(run_for(limiter, time, balanced, 400));
  EXPECT_FALSE(limiter.is_over_voltage_stopped());
}

TEST(charge_policy, pack_over_voltage_stops_the_charge)
{
  policy::ChargeLimiter limiter;
  auto in = charging_at(4000, 1000);
  in.pack_mV = policy::packOverVoltage_mV + 1;
  EXPECT_FALSE(limiter.update(1000, in));
  EXPECT_TRUE(limiter.is_over_voltage_stopped());
}

/// Unplug/replug forgets the full state, but never the over voltage
TEST(charge_policy, clear_full_keeps_the_over_voltage)
{
  policy::ChargeLimiter limiter;
  uint32_t time = 1000;
  EXPECT_FALSE(limiter.update(time, charging_cells(4113, 4106, 4110, 192)));
  limiter.clear_full();
  EXPECT_FALSE(limiter.is_full());
  EXPECT_TRUE(limiter.is_over_voltage_stopped());
  EXPECT_FALSE(limiter.update(time += 100, charging_cells(4113, 4106, 4110, 192)));
}

TEST(charge_policy, invalid_balancer_ignores_the_cell_voltages)
{
  policy::ChargeLimiter limiter;
  auto in = charging_at(4300, 1000);
  in.isBalancerValid = false;
  in.level = 5000;
  EXPECT_TRUE(limiter.update(1000, in));
}

TEST(charge_policy, time_wrap_around)
{
  policy::ChargeLimiter limiter;
  uint32_t time = UINT32_MAX - 1000;
  const auto full = charging_at(4100, 100);
  EXPECT_FALSE(run_for(limiter, time, full, policy::endOfChargeDeglitch_ms + 200));
  EXPECT_TRUE(limiter.is_full());
}

} // namespace lampda
