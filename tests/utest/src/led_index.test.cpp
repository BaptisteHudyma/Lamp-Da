#include <gtest/gtest.h>

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
  const uint16_t index = to_led_index(0.0f, -100000.0f);
  EXPECT_GE(index, LampTy::ledCount - LampTy::maxWidth - 1);
  EXPECT_LT(index, LampTy::ledCount);
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
