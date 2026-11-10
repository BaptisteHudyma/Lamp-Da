#include <gtest/gtest.h>

#include <limits>

#include "src/modes/include/hardware/lamp_type.hpp"

namespace lampda::modes {

using hardware::LampTy;

TEST(led_index, point_above_lamp_maps_to_top_row)
{
  // z is negative inside the lamp body: a positive z is above the top of the lamp
  for (const float z_mm: {0.1f, 5.0f, 50.0f, 1000.0f})
  {
    const uint16_t index = to_led_index(0.0f, z_mm);
    EXPECT_LT(index, LampTy::maxWidth) << "z_mm=" << z_mm;
    EXPECT_EQ(index, to_led_index(0.0f, 0.0f)) << "z_mm=" << z_mm;
  }
}

TEST(led_index, point_below_lamp_maps_to_bottom_row)
{
  // the bottom row is the last full turn of the strip: turn 22 starts at 22 * 25.54 = 561.9
  static_assert(LampTy::ledCount == 580, "expected indexes computed for the 580 leds indexable lamp");
  const struct
  {
    float turnFraction; // angle, as a fraction of a turn
    uint16_t expected;
  } cases[] = {
          {0.0f, 562},  // round(561.9)
          {0.25f, 568}, // round(561.9 + 6.39)
          {0.5f, 575},  // round(561.9 + 12.77)
          // past the end of the strip on the last turn: one turn up, starting at 21 * 25.54 = 536.4
          {0.75f, 556}, // round(536.4 + 19.16)
          {0.9f, 559},  // round(536.4 + 22.99)
  };

  for (const float z_mm: {-200.0f, -100000.0f})
  {
    for (const auto& testCase: cases)
    {
      const uint16_t index = to_led_index(testCase.turnFraction * c_TWO_PI, z_mm);
      EXPECT_EQ(index, testCase.expected) << "turn fraction=" << testCase.turnFraction << " z_mm=" << z_mm;
      EXPECT_GE(index, LampTy::ledCount - 2 * LampTy::maxWidth);
      EXPECT_LT(index, LampTy::ledCount);
    }
  }
}

TEST(led_index, extreme_heights_are_clamped)
{
  // far outside the int32 range: clamped before the integer conversion
  EXPECT_EQ(to_led_index(0.0f, 1e30f), to_led_index(0.0f, 0.0f));
  EXPECT_EQ(to_led_index(0.0f, -1e30f), to_led_index(0.0f, -100000.0f));
  // NaN lands on the top row
  EXPECT_EQ(to_led_index(0.0f, std::numeric_limits<float>::quiet_NaN()), to_led_index(0.0f, 0.0f));
}

TEST(led_index, strip_to_helix_rejects_invalid_index)
{
  for (const int16_t n: {static_cast<int16_t>(-1), static_cast<int16_t>(LampTy::ledCount)})
  {
    const auto helix = strip_to_helix(n);
    EXPECT_FLOAT_EQ(helix.x, 0.0f) << "n=" << n;
    EXPECT_FLOAT_EQ(helix.y, 0.0f) << "n=" << n;
    EXPECT_FLOAT_EQ(helix.z, 0.0f) << "n=" << n;
  }

  // last valid index is still converted
  const auto last = strip_to_helix(LampTy::ledCount - 1);
  EXPECT_FLOAT_EQ(last.z, to_helix_z(LampTy::ledCount - 1));
}

} // namespace lampda::modes
