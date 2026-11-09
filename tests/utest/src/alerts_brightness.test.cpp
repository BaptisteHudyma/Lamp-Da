#include <gtest/gtest.h>

#include "src/system/logic/alerts.h"
#include "src/system/logic/brightness_handle.h"

#include "src/system/utils/constants.h"

// access simulation states
#include "simulator/include/hardware_influencer.h"

namespace lampda::logic {

namespace {

static constexpr brightness_t maxBrightness = ::lampda::brightness::absoluteMaximumBrightness;
/// limit of the TEMP_TOO_HIGH alert
static constexpr brightness_t tempTooHighBrightness = static_cast<brightness_t>(0.5 * maxBrightness);
/// limit of the BATTERY_LOW alert
static constexpr brightness_t batteryLowBrightness = static_cast<brightness_t>(0.25 * maxBrightness);

/// Above the TEMP_TOO_HIGH raise threshold, below the TEMP_CRITICAL one
static constexpr float highTemperature_c = physical_constants::maxSystemTemp_c + 1.0f;
/// Below the TEMP_TOO_HIGH clear threshold
static constexpr float normalTemperature_c = 25.0f;

} // namespace

class alerts_brightness_test : public ::testing::Test
{
protected:
  void SetUp() override { reset(); }
  void TearDown() override { reset(); }

  static void reset()
  {
    ::simulator::mock_registers::cpuTemperature = normalTemperature_c;
    for (const auto type: {alerts::Type::TEMP_TOO_HIGH,
                           alerts::Type::BATTERY_LOW,
                           alerts::Type::SUNSET_TIMER_ENABLED,
                           alerts::Type::BLUETOOTH_ADVERT})
      alerts::manager.clear(type);
    brightness::set_max_brightness(maxBrightness);
  }
};

TEST_F(alerts_brightness_test, no_alert_no_limit) { EXPECT_EQ(alerts::get_alerts_max_brightness(), maxBrightness); }

TEST_F(alerts_brightness_test, unrelated_alert_no_limit)
{
  alerts::manager.raise(alerts::Type::SUNSET_TIMER_ENABLED);
  alerts::manager.raise(alerts::Type::BLUETOOTH_ADVERT);
  EXPECT_EQ(alerts::get_alerts_max_brightness(), maxBrightness);
}

TEST_F(alerts_brightness_test, lowest_limit_wins)
{
  alerts::manager.raise(alerts::Type::TEMP_TOO_HIGH);
  EXPECT_EQ(alerts::get_alerts_max_brightness(), tempTooHighBrightness);

  alerts::manager.raise(alerts::Type::BATTERY_LOW);
  EXPECT_EQ(alerts::get_alerts_max_brightness(), batteryLowBrightness);

  // the remaining limit applies
  alerts::manager.clear(alerts::Type::BATTERY_LOW);
  EXPECT_EQ(alerts::get_alerts_max_brightness(), tempTooHighBrightness);
}

/// Issue #604: a cleared clamping alert must release the limit, even with other alerts still raised
TEST_F(alerts_brightness_test, limit_released_with_other_alert_raised)
{
  alerts::manager.raise(alerts::Type::SUNSET_TIMER_ENABLED);

  // temperature alert raised by the alert loop: the output is clamped
  ::simulator::mock_registers::cpuTemperature = highTemperature_c;
  alerts::handle_all(false);
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::TEMP_TOO_HIGH));
  EXPECT_EQ(brightness::get_max_brightness(), tempTooHighBrightness);
  // second turn runs the on_raised() action
  alerts::handle_all(false);
  EXPECT_EQ(brightness::get_max_brightness(), tempTooHighBrightness);

  // temperature back to normal: the alert clears, the sunset timer alert stays
  ::simulator::mock_registers::cpuTemperature = normalTemperature_c;
  alerts::handle_all(false);
  ASSERT_FALSE(alerts::manager.is_raised(alerts::Type::TEMP_TOO_HIGH));
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::SUNSET_TIMER_ENABLED));
  EXPECT_EQ(brightness::get_max_brightness(), maxBrightness);
}

/// The limit is kept while the clamping alert is raised, whatever the alert display
TEST_F(alerts_brightness_test, limit_kept_when_ignoring_alerts)
{
  ::simulator::mock_registers::cpuTemperature = highTemperature_c;
  alerts::handle_all(true);
  alerts::handle_all(true);
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::TEMP_TOO_HIGH));
  EXPECT_EQ(brightness::get_max_brightness(), tempTooHighBrightness);
}

} // namespace lampda::logic
