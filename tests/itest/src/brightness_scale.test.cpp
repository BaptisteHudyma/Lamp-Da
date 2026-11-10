#include <gtest/gtest.h>

#include "src/system/logic/brightness_handle.h"

#include "src/system/utils/constants.h"

namespace lampda::logic::brightness {

class brightness_scale_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
    update_brightness(500, true);
    update_saved_brightness();
  }

  void TearDown() override { set_user_brightness_scale(1.0f); }
};

TEST_F(brightness_scale_test, scales_saved_brightness_and_user_limit_together)
{
  ASSERT_EQ(get_scaled_saved_brightness(), 500);
  ASSERT_EQ(get_max_user_brightness(), ::lampda::brightness::absoluteMaximumBrightness);

  set_user_brightness_scale(0.5f);
  ASSERT_EQ(get_scaled_saved_brightness(), 250);
  ASSERT_EQ(get_max_user_brightness(), ::lampda::brightness::absoluteMaximumBrightness / 2);

  // the saved value is what gets restored at the end of a sun timer
  ASSERT_EQ(get_saved_brightness(), 500);
}

TEST_F(brightness_scale_test, scale_is_limited_between_zero_and_one)
{
  set_user_brightness_scale(-1.0f);
  ASSERT_EQ(get_scaled_saved_brightness(), 0);
  ASSERT_EQ(get_max_user_brightness(), 0);

  set_user_brightness_scale(2.0f);
  ASSERT_EQ(get_scaled_saved_brightness(), 500);
  ASSERT_EQ(get_max_user_brightness(), ::lampda::brightness::absoluteMaximumBrightness);
}

TEST_F(brightness_scale_test, user_brightness_update_resets_the_scale)
{
  set_user_brightness_scale(0.5f);

  update_brightness(300, true);
  update_saved_brightness();

  ASSERT_EQ(get_scaled_saved_brightness(), 300);
  ASSERT_EQ(get_max_user_brightness(), ::lampda::brightness::absoluteMaximumBrightness);
}

TEST_F(brightness_scale_test, system_limit_caps_the_user_limit)
{
  set_max_brightness(256);
  ASSERT_EQ(get_max_user_brightness(), 256);

  set_user_brightness_scale(0.5f);
  ASSERT_EQ(get_max_user_brightness(), 128);
}

} // namespace lampda::logic::brightness
