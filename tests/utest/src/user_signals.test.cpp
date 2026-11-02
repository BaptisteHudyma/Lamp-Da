#include <gtest/gtest.h>

#include <cstring>

#include "src/system/logic/user_signals.h"

namespace lampda {
namespace logic {
namespace user_signals {

// The signal format is a public interface: those strings must never change
TEST(UserSignalsTest, StableFormat)
{
  char buffer[maxFormatLenght];

  const uint32_t brightness = 1024;
  EXPECT_EQ(format(buffer, "brgt", &brightness, 1), strlen("$brgt:1024"));
  EXPECT_STREQ(buffer, "$brgt:1024");

  const uint32_t mode[] = {3, 12};
  format(buffer, "mode", mode, 2);
  EXPECT_STREQ(buffer, "$mode:3:12");

  const uint32_t ramp = 255;
  format(buffer, "ramp", &ramp, 1);
  EXPECT_STREQ(buffer, "$ramp:255");

  const uint32_t indicator = 2;
  format(buffer, "ind", &indicator, 1);
  EXPECT_STREQ(buffer, "$ind:2");
}

TEST(UserSignalsTest, TooLongIsDropped)
{
  char buffer[maxFormatLenght];
  const uint32_t values[] = {4000000000, 4000000000, 4000000000};
  EXPECT_EQ(format(buffer, "mode", values, 3), 0u);
  EXPECT_STREQ(buffer, "");
}

} // namespace user_signals
} // namespace logic
} // namespace lampda
