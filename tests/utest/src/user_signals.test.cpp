#include <gtest/gtest.h>

#include <cstring>

#include "src/system/logic/user_signals.h"

namespace lampda {
namespace logic {
namespace user_signals {

// The signal format is a public interface: those strings must never change
TEST(UserSignalsTest, StableFormat)
{
  char buffer[maxSignalLenght];

  const uint32_t brightness = 1024;
  EXPECT_EQ(format(buffer, "brightness", &brightness, 1), strlen("$brightness:1024\r\n"));
  EXPECT_STREQ(buffer, "$brightness:1024\r\n");

  const uint32_t mode[] = {3, 12};
  format(buffer, "mode", mode, 2);
  EXPECT_STREQ(buffer, "$mode:3:12\r\n");

  const uint32_t ramp = 255;
  format(buffer, "ramp", &ramp, 1);
  EXPECT_STREQ(buffer, "$ramp:255\r\n");

  const uint32_t indicator = 2;
  format(buffer, "indicator", &indicator, 1);
  EXPECT_STREQ(buffer, "$indicator:2\r\n");
}

TEST(UserSignalsTest, TooLongIsDropped)
{
  char buffer[maxSignalLenght];
  const uint32_t values[] = {4000000000, 4000000000, 4000000000};
  EXPECT_EQ(format(buffer, "mode", values, 3), 0u);
  EXPECT_STREQ(buffer, "");
}

} // namespace user_signals
} // namespace logic
} // namespace lampda
