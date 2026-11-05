#include <gtest/gtest.h>

#include <cstdint>

#include "src/system/logic/battery_blink.h"

namespace lampda::logic::battery_blink {

namespace {

static constexpr uint32_t tick_ms = 12;
// time when a level drop is displayed at once if it is already filtered
static constexpr uint32_t afterMinUseTime_ms = minOutputUseTime_ms + levelFilterWindow_ms;

/// Run the tracker for a duration, and return the blink count
uint32_t run(BlinkTracker& tracker,
             uint32_t& time_ms,
             const uint32_t duration_ms,
             const uint16_t level,
             const bool isValid = true,
             const bool canBlink = true)
{
  uint32_t blinks = 0;
  const uint32_t endTime = time_ms + duration_ms;
  for (; time_ms < endTime; time_ms += tick_ms)
    if (tracker.update(time_ms, level, isValid, canBlink))
      blinks++;
  return blinks;
}

} // namespace

TEST(battery_blink, count_thresholds)
{
  EXPECT_EQ(get_blink_count(10000), 0);
  EXPECT_EQ(get_blink_count(7500), 0);
  EXPECT_EQ(get_blink_count(7499), 1);
  EXPECT_EQ(get_blink_count(5000), 1);
  EXPECT_EQ(get_blink_count(4999), 2);
  EXPECT_EQ(get_blink_count(2500), 2);
  EXPECT_EQ(get_blink_count(2499), 3);
  EXPECT_EQ(get_blink_count(0), 3);
}

TEST(battery_blink, never_blinks_before_min_use_time)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  // the level crossed during the use time is the reference level, it is not displayed
  EXPECT_EQ(run(tracker, time, minOutputUseTime_ms / 2, 8000), 0);
  EXPECT_EQ(run(tracker, time, minOutputUseTime_ms, 7000), 0);
}

TEST(battery_blink, level_already_passed_at_start_is_not_displayed)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, 60000, 4000), 0);
  // the next threshold is displayed normally, and with the count of this threshold
  EXPECT_EQ(run(tracker, time, 60000, 2000), 3);
}

TEST(battery_blink, each_level_blinks_once_and_never_repeats)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);

  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, 60000, 7000), 1);
  EXPECT_EQ(run(tracker, time, 60000, 4500), 2);
  EXPECT_EQ(run(tracker, time, 60000, 2000), 3);
  EXPECT_EQ(run(tracker, time, 60000, 500), 0);
}

TEST(battery_blink, blinks_are_spaced)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, levelFilterWindow_ms + 3 * tick_ms, 2000), 1);
  EXPECT_EQ(run(tracker, time, blinkPeriod_ms - 4 * tick_ms, 2000), 0);
  EXPECT_EQ(run(tracker, time, 10 * blinkPeriod_ms, 2000), 2);
}

TEST(battery_blink, drop_of_several_levels_at_once_blinks)
{
  // 2 levels at once
  {
    BlinkTracker tracker;
    uint32_t time = 0;
    tracker.reset(time);
    EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
    EXPECT_EQ(run(tracker, time, 5000, 4000), 2);
    EXPECT_EQ(run(tracker, time, 5000, 4000), 0);
  }
  // all levels at once
  {
    BlinkTracker tracker;
    uint32_t time = 0;
    tracker.reset(time);
    EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
    EXPECT_EQ(run(tracker, time, 5000, 2000), 3);
    EXPECT_EQ(run(tracker, time, 5000, 2000), 0);
  }
  // from an already displayed level
  {
    BlinkTracker tracker;
    uint32_t time = 0;
    tracker.reset(time);
    EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 7000), 0);
    EXPECT_EQ(run(tracker, time, 5000, 2000), 3);
  }
}

TEST(battery_blink, drop_during_min_use_time_is_never_displayed)
{
  // several levels
  {
    BlinkTracker tracker;
    uint32_t time = 0;
    tracker.reset(time);
    EXPECT_EQ(run(tracker, time, 5000, 8000), 0);
    EXPECT_EQ(run(tracker, time, 60000, 4000), 0);
    // the next threshold is displayed normally
    EXPECT_EQ(run(tracker, time, 5000, 2000), 3);
  }
  // just before the end of the use time
  {
    BlinkTracker tracker;
    uint32_t time = 0;
    tracker.reset(time);
    EXPECT_EQ(run(tracker, time, minOutputUseTime_ms - 500, 8000), 0);
    EXPECT_EQ(run(tracker, time, 60000, 7000), 0);
  }
}

TEST(battery_blink, voltage_recovery_does_not_blink_again)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 7000), 1);
  // current use drops, and the voltage rises over the threshold, then drops under it again
  EXPECT_EQ(run(tracker, time, 5000, 8000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 7000), 0);
}

TEST(battery_blink, short_drops_are_filtered)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  for (int i = 0; i < 10; ++i)
  {
    EXPECT_EQ(run(tracker, time, levelFilterWindow_ms - 2 * tick_ms, 7000), 0);
    EXPECT_EQ(run(tracker, time, 100, 8000), 0);
  }
}

TEST(battery_blink, can_blink_again_after_output_reentry)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 7000), 1);

  // the voltage rose while the lamp was off
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 7000), 1);

  // still under the threshold after a restart: nothing to display
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms + 5000, 7000), 0);
}

TEST(battery_blink, blocked_blinks_are_delayed_not_lost)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms, 8000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 4000, true, false), 0);
  EXPECT_EQ(run(tracker, time, 5000, 4000), 2);
}

TEST(battery_blink, invalid_level_is_ignored)
{
  BlinkTracker tracker;
  uint32_t time = 0;
  tracker.reset(time);
  EXPECT_EQ(run(tracker, time, afterMinUseTime_ms + 5000, 0, false), 0);
  // the reference level is read when the readings come back, it is not displayed
  EXPECT_EQ(run(tracker, time, levelFilterWindow_ms + 5000, 7000), 0);
  EXPECT_EQ(run(tracker, time, 5000, 4000), 2);
}

} // namespace lampda::logic::battery_blink
