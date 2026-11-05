#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "src/system/common/user_commands.h"
#include "src/system/hal/time.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/utils/constants.h"

#include "src/user/functions.h"

namespace lampda {

namespace {

static constexpr uint32_t blipDuration_ms = 50;
static constexpr brightness_t testBrightness = 500;
// more calls than the frames skipped by the blip (one is skipped per call)
static constexpr int loopCallsToEndBlip = 20;
// over the brightness update rate limit
static constexpr auto waitRateLimit = std::chrono::milliseconds(40);

void blip() { user::handle_user_command(common::UserCommand::make_blip_output_command(blipDuration_ms)); }

void run_until_blip_end()
{
  for (int i = 0; i < loopCallsToEndBlip; ++i)
    user::loop();
}

} // namespace

class blip_restore_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    logic::brightness::set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
    logic::brightness::update_brightness(testBrightness, true);
    logic::brightness::update_saved_brightness();
    user::power_on_sequence();
    std::this_thread::sleep_for(waitRateLimit);
  }
};

/// A blip started one tick after the lamp time was refreshed: the path of the battery blinks
TEST_F(blip_restore_test, brightness_restored_after_blip_on_next_tick)
{
  user::loop();
  std::this_thread::sleep_for(waitRateLimit);

  blip();
  ASSERT_EQ(logic::brightness::get_brightness(), 0);

  run_until_blip_end();
  EXPECT_EQ(logic::brightness::get_brightness(), testBrightness);
}

/**
 * A blip started in the same millisecond as the refresh of the lamp time: the brightness update of the blip has the
 * same timestamp as the lamp time, and this time is frozen during the skipped frames.
 * The rate limit of the brightness then saw an update "just now", and the lamp stayed off.
 */
TEST_F(blip_restore_test, brightness_restored_after_blip_in_same_millisecond_as_lamp_time)
{
  bool isSameMillisecond = false;
  // the millisecond can change between the calls: try again until it does not
  for (int attempt = 0; attempt < 200 and not isSameMillisecond; ++attempt)
  {
    std::this_thread::sleep_for(waitRateLimit);

    const uint32_t startTime = hal::time_ms();
    user::loop();
    blip();
    isSameMillisecond = hal::time_ms() == startTime;

    if (not isSameMillisecond)
    {
      run_until_blip_end();
      ASSERT_EQ(logic::brightness::get_brightness(), testBrightness);
    }
  }
  ASSERT_TRUE(isSameMillisecond) << "could not reproduce the timing";
  ASSERT_EQ(logic::brightness::get_brightness(), 0);

  run_until_blip_end();
  EXPECT_EQ(logic::brightness::get_brightness(), testBrightness);
}

} // namespace lampda
