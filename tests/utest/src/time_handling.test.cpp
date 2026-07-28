#include <cstdint>
#include <gtest/gtest.h>

#include "src/system/physical/time_handling.h"

#include <array>

namespace lampda::time {

TEST(test_time_handling, time_to_seconds)
{
  RealTime time;
  ASSERT_FALSE(time.is_valid());

  // time seconds
  for (uint8_t i = 0; i < 60; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = 0;
    time.minutes = 0;
    time.seconds = i;
    ASSERT_TRUE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i);
  }
  for (uint8_t i = 60; i < UINT8_MAX; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = 0;
    time.minutes = 0;
    time.seconds = i;
    ASSERT_FALSE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i);
  }

  // time minutes
  time.seconds = 0;
  for (uint8_t i = 0; i < 60; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = 0;
    time.minutes = i;
    time.seconds = 0;
    ASSERT_TRUE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60);
  }
  for (uint8_t i = 60; i < UINT8_MAX; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = 0;
    time.minutes = i;
    time.seconds = 0;
    ASSERT_FALSE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60);
  }

  // time hours
  time.minutes = 0;
  time.seconds = 0;
  for (uint8_t i = 0; i < 24; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = i;
    time.minutes = 0;
    time.seconds = 0;
    ASSERT_TRUE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60 * 60);
  }
  for (uint8_t i = 24; i < UINT8_MAX; i++)
  {
    time.dayOfTheWeek = 0;
    time.hour = i;
    time.minutes = 0;
    time.seconds = 0;
    ASSERT_FALSE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60 * 60);
  }

  // time days
  time.hour = 0;
  time.minutes = 0;
  time.seconds = 0;
  for (uint8_t i = 0; i < 7; i++)
  {
    time.dayOfTheWeek = i;
    time.hour = 0;
    time.minutes = 0;
    time.seconds = 0;
    ASSERT_TRUE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60 * 60 * 24);
  }
  for (uint8_t i = 7; i < UINT8_MAX; i++)
  {
    time.dayOfTheWeek = i;
    time.hour = 0;
    time.minutes = 0;
    time.seconds = 0;
    ASSERT_FALSE(time.is_valid());
    ASSERT_EQ(time.as_seconds(), i * 60 * 60 * 24);
  }
}

TEST(test_time_handling, time_to_seconds_known_values)
{
  RealTime time;
  ASSERT_FALSE(time.is_valid());

  time.dayOfTheWeek = 0;
  time.hour = 0;
  time.minutes = 0;
  time.seconds = 0;
  ASSERT_TRUE(time.is_valid());
  ASSERT_EQ(time.as_seconds(), 0);

  time.dayOfTheWeek = 0;
  time.hour = 1;
  time.minutes = 1;
  time.seconds = 1;
  ASSERT_TRUE(time.is_valid());
  ASSERT_EQ(time.as_seconds(), 3661);

  time.dayOfTheWeek = 1;
  time.hour = 1;
  time.minutes = 1;
  time.seconds = 1;
  ASSERT_TRUE(time.is_valid());
  ASSERT_EQ(time.as_seconds(), 90061);

  time.dayOfTheWeek = 6;
  time.hour = 1;
  time.minutes = 1;
  time.seconds = 1;
  ASSERT_TRUE(time.is_valid());
  ASSERT_EQ(time.as_seconds(), 522061);
}

} // namespace lampda::time
