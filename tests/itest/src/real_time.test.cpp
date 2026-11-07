#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "src/system/component/time_handling.h"

#include "src/system/hal/time.h"

#include "src/system/logic/inputs.h"

#include "src/system/bsp/threads.h"

// access simulation states
#include "simulator/include/simulator_state.h"
#include "simulator/include/hardware_influencer.h"

namespace lampda {

using namespace std::chrono_literals;

class RealTimeFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    //
    simulator::mock_registers::shouldStopThreads = false;
    ::simulator::time_mocks::reset();
  }

  void TearDown() override
  {
    // shutdown all threads
    bsp::threads::shutdown();
  }

private:
};

// Mock time from the start
TEST_F(RealTimeFixture, set_from_zero)
{
  ASSERT_EQ(hal::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 0;
  _time.hour = 0;
  _time.minutes = 0;
  _time.seconds = 0;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(hal::time_s(), i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
    ASSERT_EQ(_time.dayOfTheWeek, 0);
    ASSERT_EQ(_time.hour, 0);
    ASSERT_EQ(_time.minutes, 0);
    ASSERT_EQ(_time.seconds, i + 1);
  }
}

// Pick the soonest enabled day of a days mask
TEST_F(RealTimeFixture, next_enabled_day)
{
  using component::time::get_next_enabled_day;

  // wednesday 10h00
  component::time::RealTime _time;
  _time.dayOfTheWeek = 2;
  _time.hour = 10;
  _time.minutes = 0;
  _time.seconds = 0;
  ASSERT_TRUE(component::time::set_real_time(_time));

  constexpr uint8_t monday = 1 << 0;
  constexpr uint8_t wednesday = 1 << 2;
  constexpr uint8_t friday = 1 << 4;
  constexpr uint8_t sunday = 1 << 6;

  // no day enabled
  ASSERT_EQ(get_next_enabled_day(0, 9, 0, 0), -1);
  // only the enable/disable bit
  ASSERT_EQ(get_next_enabled_day(1 << 7, 9, 0, 0), -1);

  // later today
  ASSERT_EQ(get_next_enabled_day(monday | wednesday | friday, 11, 0, 0), 2);
  // already passed today: next enabled day, not the lowest bit
  ASSERT_EQ(get_next_enabled_day(monday | wednesday | friday, 9, 0, 0), 4);
  // wraps to next week
  ASSERT_EQ(get_next_enabled_day(monday | sunday, 9, 0, 0), 6);
  ASSERT_EQ(get_next_enabled_day(monday, 9, 0, 0), 0);
  // only today, already passed: today next week
  ASSERT_EQ(get_next_enabled_day(wednesday, 9, 0, 0), 2);
  // bit 7 is ignored
  ASSERT_EQ(get_next_enabled_day((1 << 7) | friday, 9, 0, 0), 4);
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeFixture, set_from_zero_real_time)
{
  ASSERT_EQ(hal::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 2;
  _time.hour = 2;
  _time.minutes = 2;
  _time.seconds = 2;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(hal::time_s(), i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
    ASSERT_EQ(_time.dayOfTheWeek, 2);
    ASSERT_EQ(_time.hour, 2);
    ASSERT_EQ(_time.minutes, 2);
    ASSERT_EQ(_time.seconds, 2 + i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeFixture, set_from_zero_with_wrap)
{
  ASSERT_EQ(hal::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 1;
  _time.hour = 23;
  _time.minutes = 59;
  _time.seconds = 58;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  // first second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(hal::time_s(), 1);

  _time = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 1);
  ASSERT_EQ(_time.hour, 23);
  ASSERT_EQ(_time.minutes, 59);
  ASSERT_EQ(_time.seconds, 59);

  // two second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(hal::time_s(), 2);

  _time = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 2);
  ASSERT_EQ(_time.hour, 0);
  ASSERT_EQ(_time.minutes, 0);
  ASSERT_EQ(_time.seconds, 0);
}

class RealTimeWithOffsetFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    //
    simulator::mock_registers::shouldStopThreads = false;
    ::simulator::time_mocks::reset(offset_time_s * 1000);
  }

  void TearDown() override
  {
    // shutdown all threads
    bsp::threads::shutdown();
  }

public:
  static constexpr uint32_t offset_time_s = 10;
};

// Mock time from the start
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds)
{
  ASSERT_EQ(hal::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 0;
  _time.hour = 0;
  _time.minutes = 0;
  _time.seconds = 0;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));

  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(hal::time_s(), offset_time_s + i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));

    ASSERT_EQ(_time.dayOfTheWeek, 0);
    ASSERT_EQ(_time.hour, 0);
    ASSERT_EQ(_time.minutes, 0);
    ASSERT_EQ(_time.seconds, i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds_real_time)
{
  ASSERT_EQ(hal::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 2;
  _time.hour = 2;
  _time.minutes = 2;
  _time.seconds = 2;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(hal::time_s(), offset_time_s + i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));

    ASSERT_EQ(_time.dayOfTheWeek, 2);
    ASSERT_EQ(_time.hour, 2);
    ASSERT_EQ(_time.minutes, 2);
    ASSERT_EQ(_time.seconds, 2 + i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds_with_wrap)
{
  ASSERT_EQ(hal::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 1;
  _time.hour = 23;
  _time.minutes = 59;
  _time.seconds = 58;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  // first second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(hal::time_s(), offset_time_s + 1);

  _time = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 1);
  ASSERT_EQ(_time.hour, 23);
  ASSERT_EQ(_time.minutes, 59);
  ASSERT_EQ(_time.seconds, 59);

  // two second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(hal::time_s(), offset_time_s + 2);

  _time = component::time::get_real_time();
  ASSERT_EQ(hal::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 2);
  ASSERT_EQ(_time.hour, 0);
  ASSERT_EQ(_time.minutes, 0);
  ASSERT_EQ(_time.seconds, 0);
}

namespace {

constexpr uint32_t secondsPerDay = 24 * 3600;
constexpr uint32_t secondsPerWeek = 7 * secondsPerDay;

component::time::RealTime make_time(uint8_t day, uint8_t hour, uint8_t minutes, uint8_t seconds)
{
  component::time::RealTime time;
  time.dayOfTheWeek = day;
  time.hour = hour;
  time.minutes = minutes;
  time.seconds = seconds;
  return time;
}

/// Check that the target platform time is in the future (or now), at most a week away, and on the expected real time
void expect_next_occurrence(const component::time::RealTime& target, const uint32_t expectedDelay_s)
{
  const uint32_t now_s = hal::time_s();
  const uint32_t platformTime_s = get_platform_time_from_target_time(target);
  ASSERT_GE(platformTime_s, now_s);
  // the simulated clock runs: a second can pass since the time synchronization
  ASSERT_NEAR(static_cast<double>(platformTime_s - now_s), static_cast<double>(expectedDelay_s), 2.0);

  const auto& reached = component::time::convert_to_real_time(platformTime_s);
  ASSERT_EQ(reached.dayOfTheWeek, target.dayOfTheWeek);
  ASSERT_EQ(reached.hour, target.hour);
  ASSERT_EQ(reached.minutes, target.minutes);
  ASSERT_EQ(reached.seconds, target.seconds);
}

} // namespace

// Targets later in the week, and earlier in the week (next week)
TEST_F(RealTimeFixture, target_next_occurrence)
{
  // wednesday 12:00:00, just after the start
  ASSERT_TRUE(component::time::set_real_time(make_time(2, 12, 0, 0)));

  // later the same day
  expect_next_occurrence(make_time(2, 13, 0, 0), 3600);
  // later in the week
  expect_next_occurrence(make_time(4, 12, 0, 0), 2 * secondsPerDay);
  // earlier the same day: next week
  expect_next_occurrence(make_time(2, 11, 0, 0), secondsPerWeek - 3600);
  // earlier in the week: next week
  expect_next_occurrence(make_time(0, 8, 0, 0), 5 * secondsPerDay - 4 * 3600);
}

// Same targets, with a lamp running for more than a week before the synchronization
TEST_F(RealTimeFixture, target_next_occurrence_long_uptime)
{
  ::simulator::time_mocks::reset((secondsPerWeek + 3 * secondsPerDay + 17) * 1000);

  // wednesday 12:00:00
  ASSERT_TRUE(component::time::set_real_time(make_time(2, 12, 0, 0)));

  expect_next_occurrence(make_time(2, 13, 0, 0), 3600);
  expect_next_occurrence(make_time(4, 12, 0, 0), 2 * secondsPerDay);
  expect_next_occurrence(make_time(2, 11, 0, 0), secondsPerWeek - 3600);
  expect_next_occurrence(make_time(0, 8, 0, 0), 5 * secondsPerDay - 4 * 3600);
}

} // namespace lampda
