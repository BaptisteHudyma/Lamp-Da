#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "src/system/physical/time_handling.h"

#include "src/system/logic/inputs.h"

#include "src/system/platform/threads.h"
#include "src/system/platform/time.h"

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
    platform::threads::shutdown();
  }

private:
};

// Mock time from the start
TEST_F(RealTimeFixture, set_from_zero)
{
  ASSERT_EQ(platform::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 0;
  _time.hour = 0;
  _time.minutes = 0;
  _time.seconds = 0;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(platform::time_s(), i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
    ASSERT_EQ(_time.dayOfTheWeek, 0);
    ASSERT_EQ(_time.hour, 0);
    ASSERT_EQ(_time.minutes, 0);
    ASSERT_EQ(_time.seconds, i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeFixture, set_from_zero_real_time)
{
  ASSERT_EQ(platform::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 2;
  _time.hour = 2;
  _time.minutes = 2;
  _time.seconds = 2;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(platform::time_s(), i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
    ASSERT_EQ(_time.dayOfTheWeek, 2);
    ASSERT_EQ(_time.hour, 2);
    ASSERT_EQ(_time.minutes, 2);
    ASSERT_EQ(_time.seconds, 2 + i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeFixture, set_from_zero_with_wrap)
{
  ASSERT_EQ(platform::time_s(), 0);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 1;
  _time.hour = 23;
  _time.minutes = 59;
  _time.seconds = 58;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  // first second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(platform::time_s(), 1);

  _time = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 1);
  ASSERT_EQ(_time.hour, 23);
  ASSERT_EQ(_time.minutes, 59);
  ASSERT_EQ(_time.seconds, 59);

  // two second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(platform::time_s(), 2);

  _time = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
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
    platform::threads::shutdown();
  }

public:
  static constexpr uint32_t offset_time_s = 10;
};

// Mock time from the start
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds)
{
  ASSERT_EQ(platform::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 0;
  _time.hour = 0;
  _time.minutes = 0;
  _time.seconds = 0;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));

  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(platform::time_s(), offset_time_s + i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));

    ASSERT_EQ(_time.dayOfTheWeek, 0);
    ASSERT_EQ(_time.hour, 0);
    ASSERT_EQ(_time.minutes, 0);
    ASSERT_EQ(_time.seconds, i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds_real_time)
{
  ASSERT_EQ(platform::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 2;
  _time.hour = 2;
  _time.minutes = 2;
  _time.seconds = 2;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  for (uint8_t i = 0; i < 3; i++)
  {
    std::this_thread::sleep_for(1s);
    ASSERT_EQ(platform::time_s(), offset_time_s + i + 1);

    const auto& _time = component::time::get_real_time();
    ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));

    ASSERT_EQ(_time.dayOfTheWeek, 2);
    ASSERT_EQ(_time.hour, 2);
    ASSERT_EQ(_time.minutes, 2);
    ASSERT_EQ(_time.seconds, 2 + i + 1);
  }
}

// Mock time fromt he start, with a bigger offset
TEST_F(RealTimeWithOffsetFixture, set_from_10seconds_with_wrap)
{
  ASSERT_EQ(platform::time_s(), offset_time_s);

  component::time::RealTime _time;
  _time.dayOfTheWeek = 1;
  _time.hour = 23;
  _time.minutes = 59;
  _time.seconds = 58;
  ASSERT_TRUE(component::time::set_real_time(_time));

  const auto& startTime = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(startTime));
  ASSERT_EQ(startTime.dayOfTheWeek, _time.dayOfTheWeek);
  ASSERT_EQ(startTime.hour, _time.hour);
  ASSERT_EQ(startTime.minutes, _time.minutes);
  ASSERT_EQ(startTime.seconds, _time.seconds);

  // first second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(platform::time_s(), offset_time_s + 1);

  _time = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 1);
  ASSERT_EQ(_time.hour, 23);
  ASSERT_EQ(_time.minutes, 59);
  ASSERT_EQ(_time.seconds, 59);

  // two second
  std::this_thread::sleep_for(1s);
  ASSERT_EQ(platform::time_s(), offset_time_s + 2);

  _time = component::time::get_real_time();
  ASSERT_EQ(platform::time_s(), get_platform_time_from_target_time(_time));
  ASSERT_EQ(_time.dayOfTheWeek, 2);
  ASSERT_EQ(_time.hour, 0);
  ASSERT_EQ(_time.minutes, 0);
  ASSERT_EQ(_time.seconds, 0);
}

} // namespace lampda
