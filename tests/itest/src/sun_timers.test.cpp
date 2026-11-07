#include <gtest/gtest.h>

#include "src/system/hal/time.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sun_timers.h"

#include "src/system/utils/constants.h"

// access simulation states
#include "simulator/include/hardware_influencer.h"

namespace lampda::logic {

/// State of a timer when its end of timer handle is entered
struct EndOfTimerSnapshot
{
  bool wasCalled = false;
  bool wasEnabled = true;
  bool wasAllowedToControlBrightness = false;
  bool wasAlertRaised = true;
  brightness_t maxUserBrightness = 0;
};

static EndOfTimerSnapshot take_snapshot(const bool isEnabled, const bool isAllowedToControlBrightness)
{
  EndOfTimerSnapshot snapshot;
  snapshot.wasCalled = true;
  snapshot.wasEnabled = isEnabled;
  snapshot.wasAllowedToControlBrightness = isAllowedToControlBrightness;
  snapshot.wasAlertRaised = alerts::manager.is_raised(alerts::Type::SUNSET_TIMER_ENABLED);
  snapshot.maxUserBrightness = brightness::get_max_user_brightness();
  return snapshot;
}

class TestSunsetTimer : public SunsetTimer
{
public:
  EndOfTimerSnapshot atEnd;
  brightness_t afterEndMaxUserBrightness = 0;

protected:
  // the start conditions depend on the global system state, left by the other tests: not tested here
  bool should_accept_start() const override { return true; }

  void end_of_timer_handle() override
  {
    atEnd = take_snapshot(is_enabled(), isAllowedToControlBrightness);
    SunsetTimer::end_of_timer_handle();
    afterEndMaxUserBrightness = brightness::get_max_user_brightness();
  }
};

class TestSunriseTimer : public SunriseTimer
{
public:
  EndOfTimerSnapshot atEnd;
  brightness_t afterEndMaxUserBrightness = 0;

protected:
  // the start conditions depend on the global system state, left by the other tests: not tested here
  bool should_accept_start() const override { return true; }

  void end_of_timer_handle() override
  {
    atEnd = take_snapshot(is_enabled(), isAllowedToControlBrightness);
    SunriseTimer::end_of_timer_handle();
    afterEndMaxUserBrightness = brightness::get_max_user_brightness();
  }
};

class sun_timers_test : public ::testing::Test
{
protected:
  static constexpr brightness_t maxBrightness = ::lampda::brightness::absoluteMaximumBrightness;
  static constexpr uint32_t timerDuration_s = 2;

  void SetUp() override
  {
    // start far from zero, the timers do not accept a deadline in the past
    ::simulator::time_mocks::reset(10000);

    brightness::set_max_brightness(maxBrightness);
    brightness::update_brightness(500, true);
    brightness::update_saved_brightness();
    alerts::manager.clear(alerts::Type::SUNSET_TIMER_ENABLED);
  }

  void TearDown() override
  {
    brightness::set_user_brightness_scale(1.0f);
    alerts::manager.clear(alerts::Type::SUNSET_TIMER_ENABLED);
    behavior::set_power_off();
  }

  /// Run the timer task until the timer is released, or the timeout is reached
  template<typename Timer> static void run_until_released(Timer& timer)
  {
    const uint32_t timeout_ms = hal::time_ms() + (timerDuration_s + 2) * 1000;
    while (timer.is_enabled() and hal::time_ms() < timeout_ms)
    {
      timer.sun_process_loop();
    }
  }
};

TEST_F(sun_timers_test, sunset_end_releases_timer_without_restoring_brightness)
{
  TestSunsetTimer timer;
  timer.set_deadline(hal::time_s() + timerDuration_s);
  ASSERT_TRUE(timer.is_enabled());
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::SUNSET_TIMER_ENABLED));

  // the brightness lock must be released at the end of the timer
  timer.lock_brightness_update(true);
  brightness::set_user_brightness_scale(0.5f);

  run_until_released(timer);

  ASSERT_FALSE(timer.is_enabled());
  ASSERT_TRUE(timer.atEnd.wasCalled);

  // the timer is released before the end of timer handle runs
  EXPECT_FALSE(timer.atEnd.wasEnabled);
  EXPECT_TRUE(timer.atEnd.wasAllowedToControlBrightness);
  EXPECT_FALSE(timer.atEnd.wasAlertRaised);

  // the release did not restore the full brightness scale: no flash before the shutdown
  EXPECT_LT(timer.atEnd.maxUserBrightness, maxBrightness);

  // the end of timer handle leaves the lamp dark, and powers it off
  EXPECT_EQ(timer.afterEndMaxUserBrightness, 0);
  EXPECT_EQ(brightness::get_max_user_brightness(), 0);
  EXPECT_FALSE(behavior::is_system_should_be_powered());
}

TEST_F(sun_timers_test, sunrise_end_releases_timer_then_restores_brightness)
{
  TestSunriseTimer timer;
  timer.set_deadline(hal::time_s() + timerDuration_s);
  ASSERT_TRUE(timer.is_enabled());

  timer.lock_brightness_update(true);
  brightness::set_user_brightness_scale(0.5f);

  run_until_released(timer);

  ASSERT_FALSE(timer.is_enabled());
  ASSERT_TRUE(timer.atEnd.wasCalled);

  EXPECT_FALSE(timer.atEnd.wasEnabled);
  EXPECT_TRUE(timer.atEnd.wasAllowedToControlBrightness);
  EXPECT_LT(timer.atEnd.maxUserBrightness, maxBrightness);

  // the sunrise ends at the full saved brightness
  EXPECT_EQ(timer.afterEndMaxUserBrightness, maxBrightness);
  EXPECT_EQ(brightness::get_scaled_saved_brightness(), 500);
}

TEST_F(sun_timers_test, cancel_restores_brightness_without_end_of_timer_handle)
{
  TestSunsetTimer timer;
  timer.set_deadline(hal::time_s() + timerDuration_s);
  brightness::set_user_brightness_scale(0.5f);

  ASSERT_TRUE(timer.cancel());

  EXPECT_FALSE(timer.is_enabled());
  EXPECT_FALSE(timer.atEnd.wasCalled);
  EXPECT_FALSE(alerts::manager.is_raised(alerts::Type::SUNSET_TIMER_ENABLED));
  EXPECT_EQ(brightness::get_max_user_brightness(), maxBrightness);

  // the task does not run the end of timer handle for a canceled timer
  timer.sun_process_loop();
  EXPECT_FALSE(timer.atEnd.wasCalled);
}

TEST_F(sun_timers_test, cancel_of_stopped_timer_does_not_touch_brightness)
{
  TestSunsetTimer timer;
  brightness::set_user_brightness_scale(0.5f);

  ASSERT_FALSE(timer.cancel());

  EXPECT_EQ(brightness::get_max_user_brightness(), maxBrightness / 2);
}

} // namespace lampda::logic
