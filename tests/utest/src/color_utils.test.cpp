
#include "src/modes/include/colors/utils.hpp"

#include <limits>
#include <gtest/gtest.h>

namespace lampda::modes::colors {

TEST(test_colors, color_blend16)
{
  uint32_t c1 = 0x000000;
  uint32_t c2 = 0xFFFFFF;
  ASSERT_EQ(blend(c1, c2, 0, true), 0x000000);
  ASSERT_EQ(blend(c1, c2, UINT16_MAX / 2, true), 0x7F7F7F);
  ASSERT_EQ(blend(c1, c2, UINT16_MAX, true), 0xFFFFFF);
}

TEST(test_colors, color_blend8)
{
  uint32_t c1 = 0x000000;
  uint32_t c2 = 0xFFFFFF;
  ASSERT_EQ(blend(c1, c2, 0, false), 0x00000000);
  ASSERT_EQ(blend(c1, c2, UINT8_MAX / 2, false), 0x7E7E7E);
  ASSERT_EQ(blend(c1, c2, UINT8_MAX, false), 0xFFFFFF);
}

} // namespace lampda::modes::colors
