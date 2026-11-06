#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "src/system/logic/brightness_handle.h"
#include "src/system/utils/constants.h"

#include "src/user/functions.h"

#include "src/modes/include/hardware/lamp_type.hpp"

namespace lampda::user::_private {
extern modes::hardware::LampTy lamp;
}

namespace lampda {

namespace {

static constexpr brightness_t startBrightness = 300;
// limit of the battery low alert
static constexpr brightness_t alertMaxBrightness = 256;
static constexpr brightness_t requestedBrightness = ::lampda::brightness::absoluteMaximumBrightness;
// enough frames for the active mode to apply its brightness
static constexpr int framesToSettle = 20;
// over the brightness update rate limit
static constexpr auto waitRateLimit = std::chrono::milliseconds(40);

/// Same sequence as a brightness command (CLI, bluetooth) or a brightness navigation step
void system_set_brightness(const brightness_t brightness)
{
  logic::brightness::update_brightness(brightness);
  logic::brightness::update_saved_brightness();
}

void run_frames()
{
  for (int i = 0; i < framesToSettle; ++i)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    user::loop();
  }
}

} // namespace

class brightness_command_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    logic::brightness::set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
    logic::brightness::update_brightness(startBrightness, true);
    logic::brightness::update_saved_brightness();
    user::power_on_sequence();
    std::this_thread::sleep_for(waitRateLimit);
    run_frames();
  }

  void TearDown() override
  {
    logic::brightness::set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
  }
};

/// The first brightness command must be applied, not reverted by the mode to the previous brightness
TEST_F(brightness_command_test, first_command_is_applied)
{
  system_set_brightness(requestedBrightness);
  // modes animating around the saved brightness of the lamp (eg: candle) would revert to the previous brightness
  EXPECT_EQ(user::_private::lamp.getSavedBrightness(), requestedBrightness);
  EXPECT_EQ(user::_private::lamp.getBrightness(), requestedBrightness);

  run_frames();

  EXPECT_EQ(logic::brightness::get_brightness(), requestedBrightness);
  EXPECT_EQ(logic::brightness::get_saved_brightness(), requestedBrightness);
}

/// After an alert limited the brightness and was cleared, the first command must restore a high brightness
TEST_F(brightness_command_test, first_command_after_alert_cleared_is_applied)
{
  // alert raised: limit the brightness, as Alert_BatteryLow::on_raised
  logic::brightness::set_max_brightness(alertMaxBrightness);
  system_set_brightness(logic::brightness::get_saved_brightness());
  run_frames();
  ASSERT_EQ(logic::brightness::get_brightness(), alertMaxBrightness);

  // alert cleared
  logic::brightness::set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
  run_frames();

  std::this_thread::sleep_for(waitRateLimit);
  system_set_brightness(requestedBrightness);
  EXPECT_EQ(user::_private::lamp.getSavedBrightness(), requestedBrightness);
  run_frames();

  EXPECT_EQ(logic::brightness::get_brightness(), requestedBrightness);
}

} // namespace lampda
