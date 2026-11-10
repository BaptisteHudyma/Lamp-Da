#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "src/system/common/user_commands.h"
#include "src/system/hal/time.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/utils/constants.h"

#include "src/user/constants.h"
#include "src/user/functions.h"

namespace lampda {

namespace {

static constexpr uint32_t blipDuration_ms = 50;
static constexpr brightness_t testBrightness = 500;
// below the led masking threshold of the lamp
static constexpr brightness_t maskedBrightness = 100;
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

/// Run one main loop frame and the display thread, as the system does
void run_displayed_frame()
{
  user::loop();
  user::user_thread();
}

/// Count of the leds that are not black in the displayed output
uint16_t count_displayed_lit_leds()
{
  uint16_t lit = 0;
  for (uint16_t i = 0; i < LED_COUNT; ++i)
  {
    if (user::_private::strip.getRawPixelColor(i) != 0)
      ++lit;
  }
  return lit;
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

  run_until_blip_end();
  EXPECT_EQ(logic::brightness::get_brightness(), testBrightness);
}

/**
 * A blip started in the same millisecond as the refresh of the lamp time, and the lamp time is frozen during the
 * skipped frames: the rate limit of the brightness must not leave the lamp off.
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

  run_until_blip_end();
  EXPECT_EQ(logic::brightness::get_brightness(), testBrightness);
}

/// The leds must be off during the blip, and be back after it
TEST_F(blip_restore_test, displayed_pixels_off_during_blip_and_restored)
{
  run_displayed_frame();
  const uint16_t litBeforeBlip = count_displayed_lit_leds();
  ASSERT_GT(litBeforeBlip, 0);
  std::this_thread::sleep_for(waitRateLimit);

  blip();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), 0);

  run_until_blip_end();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), litBeforeBlip);
}

/// A blip just after a brightness update is rate limited in the brightness logic, it must still turn off the leds
TEST_F(blip_restore_test, displayed_pixels_off_during_blip_after_recent_brightness_update)
{
  run_displayed_frame();
  const uint16_t litBeforeBlip = count_displayed_lit_leds();
  ASSERT_GT(litBeforeBlip, 0);

  logic::brightness::update_brightness(testBrightness, true);
  blip();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), 0);

  run_until_blip_end();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), litBeforeBlip);
}

/// Powering on during a blip must not leave the strip black
TEST_F(blip_restore_test, displayed_pixels_restored_after_power_on_during_blip)
{
  run_displayed_frame();
  const uint16_t litBeforeBlip = count_displayed_lit_leds();
  ASSERT_GT(litBeforeBlip, 0);
  std::this_thread::sleep_for(waitRateLimit);

  blip();
  run_displayed_frame();
  ASSERT_EQ(count_displayed_lit_leds(), 0);

  user::power_off_sequence();
  user::power_on_sequence();
  run_until_blip_end();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), litBeforeBlip);
}

/// Below the masking threshold, a part of the leds is masked: the blip must not depend on this mask
TEST_F(blip_restore_test, displayed_pixels_off_during_blip_with_led_masking)
{
  logic::brightness::update_brightness(maskedBrightness, true);
  logic::brightness::update_saved_brightness();
  user::power_on_sequence();
  std::this_thread::sleep_for(waitRateLimit);

  run_displayed_frame();
  const uint16_t litBeforeBlip = count_displayed_lit_leds();
  ASSERT_GT(litBeforeBlip, 0);
  ASSERT_LT(litBeforeBlip, LED_COUNT) << "the led mask should be active";
  std::this_thread::sleep_for(waitRateLimit);

  blip();
  run_displayed_frame();
  EXPECT_EQ(count_displayed_lit_leds(), 0);

  // the masked leds must stay off during the restoration, on every frame
  for (int i = 0; i < loopCallsToEndBlip; ++i)
  {
    run_displayed_frame();
    EXPECT_LE(count_displayed_lit_leds(), litBeforeBlip) << "at frame " << i;
  }
  EXPECT_EQ(count_displayed_lit_leds(), litBeforeBlip);
}

} // namespace lampda
