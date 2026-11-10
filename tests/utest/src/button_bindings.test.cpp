#include <gtest/gtest.h>

#include "simulator/include/hardware_influencer.h"

#include "src/system/bsp/filesystem.h"
#include "src/system/component/button.h"
#include "src/system/hal/time.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/button_bindings.h"
#include "src/system/logic/inputs.h"
#include "src/system/utils/utils.h"

#include "src/user/functions.h"

namespace lampda {
namespace logic {
namespace button_bindings {

class ButtonBindingsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    bsp::filesystem::user::clear_cached();
    reset_to_defaults();
  }
  void TearDown() override
  {
    bsp::filesystem::user::clear_cached();
    reset_to_defaults();
  }

  /// Same key as button_bindings.cpp: a change breaks the links saved on the lamps
  static uint32_t storage_key() { return utils::hash("btnBindings") & 0x7fffffffu; }
};

TEST_F(ButtonBindingsTest, defaults_match_the_historical_commands)
{
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 6);
  EXPECT_EQ(get_clicks(Behavior::Sunset), 5);
  EXPECT_EQ(get_clicks(Behavior::CustomRamp), 3);
  EXPECT_EQ(get_clicks(Behavior::Indicator), 7);

  EXPECT_EQ(get_clicks(Behavior::Scroll), 6);

  EXPECT_EQ(get_hold_behavior(3), Behavior::CustomRamp);
  EXPECT_EQ(get_click_behavior(3), Behavior::None);
  EXPECT_EQ(get_click_behavior(4), Behavior::Favorite);
  EXPECT_EQ(get_hold_behavior(4), Behavior::Favorite);
  EXPECT_EQ(get_hold_behavior(5), Behavior::Sunset);
  EXPECT_EQ(get_click_behavior(5), Behavior::None);
  EXPECT_EQ(get_click_behavior(6), Behavior::QuickNavigation);
  EXPECT_EQ(get_hold_behavior(6), Behavior::Scroll);
  EXPECT_EQ(get_click_behavior(7), Behavior::Indicator);
  EXPECT_EQ(get_hold_behavior(7), Behavior::Indicator);
  EXPECT_EQ(get_click_behavior(8), Behavior::None);
  EXPECT_EQ(get_hold_behavior(8), Behavior::None);
}

TEST_F(ButtonBindingsTest, base_and_fixed_commands_cannot_be_used)
{
  for (const uint8_t clicks: {0, 1, 2, 10, 11, 13, 20, 255})
  {
    EXPECT_EQ(set_binding(Behavior::Favorite, clicks), SetResult::ReservedClicks) << int(clicks);
    EXPECT_EQ(get_click_behavior(clicks), Behavior::None) << int(clicks);
    EXPECT_EQ(get_hold_behavior(clicks), Behavior::None) << int(clicks);
  }
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);
}

TEST_F(ButtonBindingsTest, moving_frees_the_previous_clicks)
{
  EXPECT_EQ(set_binding(Behavior::Favorite, 8), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 8);
  EXPECT_EQ(get_click_behavior(8), Behavior::Favorite);
  EXPECT_EQ(get_hold_behavior(8), Behavior::Favorite);
  EXPECT_EQ(get_click_behavior(4), Behavior::None);
  EXPECT_EQ(get_hold_behavior(4), Behavior::None);

  // now usable by another behavior
  EXPECT_EQ(set_binding(Behavior::QuickNavigation, 4), SetResult::Success);
  EXPECT_EQ(get_click_behavior(4), Behavior::QuickNavigation);
}

TEST_F(ButtonBindingsTest, used_clicks_swap_the_two_behaviors)
{
  EXPECT_EQ(set_binding(Behavior::Favorite, 6), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 6);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 4);
  EXPECT_EQ(get_click_behavior(6), Behavior::Favorite);
  EXPECT_EQ(get_hold_behavior(6), Behavior::Favorite);
  EXPECT_EQ(get_click_behavior(4), Behavior::QuickNavigation);
  // favorite has both actions: scroll had to move as well
  EXPECT_EQ(get_hold_behavior(4), Behavior::Scroll);

  // the others did not move
  EXPECT_EQ(get_clicks(Behavior::Sunset), 5);
  EXPECT_EQ(get_clicks(Behavior::CustomRamp), 3);
  EXPECT_EQ(get_clicks(Behavior::Indicator), 7);

  // a behavior does not swap with itself
  EXPECT_EQ(set_binding(Behavior::Favorite, 6), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 6);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 4);
}

TEST_F(ButtonBindingsTest, none_is_not_a_behavior)
{
  EXPECT_EQ(set_binding(Behavior::None, 8), SetResult::UnsupportedBehavior);
  EXPECT_EQ(get_clicks(Behavior::None), 0);
}

TEST_F(ButtonBindingsTest, names)
{
  for (const auto behavior: {Behavior::Favorite,
                             Behavior::QuickNavigation,
                             Behavior::Sunset,
                             Behavior::CustomRamp,
                             Behavior::Indicator,
                             Behavior::Scroll})
    EXPECT_EQ(get_behavior_from_name(get_name(behavior)), behavior);
  EXPECT_EQ(get_behavior_from_name("none"), Behavior::None);
  EXPECT_EQ(get_behavior_from_name("fav"), Behavior::None);
}

TEST_F(ButtonBindingsTest, links_survive_a_save_and_load)
{
  ASSERT_EQ(set_binding(Behavior::Favorite, 9), SetResult::Success);
  ASSERT_EQ(set_binding(Behavior::QuickNavigation, 4), SetResult::Success);
  write_to_memory();

  reset_to_defaults();
  ASSERT_EQ(get_clicks(Behavior::Favorite), 4);

  load_from_memory();
  EXPECT_EQ(get_clicks(Behavior::Favorite), 9);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 4);
  EXPECT_EQ(get_clicks(Behavior::Sunset), 5);
}

TEST_F(ButtonBindingsTest, missing_values_load_the_defaults)
{
  ASSERT_EQ(set_binding(Behavior::Favorite, 9), SetResult::Success);
  load_from_memory();
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);
}

TEST_F(ButtonBindingsTest, incoherent_values_load_the_defaults)
{
  const auto pack =
          [](uint32_t favorite, uint32_t nav, uint32_t sunset, uint32_t ramp, uint32_t indicator, uint32_t scroll) {
            return favorite | (nav << 4) | (sunset << 8) | (ramp << 12) | (indicator << 16) | (scroll << 20);
          };

  // reserved clicks, and two behaviors on the same clicks
  for (const uint32_t stored: {pack(1, 6, 5, 3, 7, 6),
                               pack(4, 2, 5, 3, 7, 6),
                               pack(4, 6, 0, 3, 7, 6),
                               pack(8, 8, 5, 3, 7, 6),
                               pack(4, 6, 5, 10, 7, 6),
                               pack(4, 6, 5, 3, 3, 6),
                               pack(3, 6, 5, 8, 7, 6),
                               pack(4, 6, 8, 3, 7, 8),
                               pack(4, 6, 5, 3, 7, 0)})
  {
    bsp::filesystem::user::set_value(storage_key(), stored);
    ASSERT_EQ(set_binding(Behavior::Favorite, 9), SetResult::Success);

    load_from_memory();
    EXPECT_EQ(get_clicks(Behavior::Favorite), 4) << stored;
    EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 6) << stored;
    EXPECT_EQ(get_clicks(Behavior::Sunset), 5) << stored;
    EXPECT_EQ(get_clicks(Behavior::CustomRamp), 3) << stored;
    EXPECT_EQ(get_clicks(Behavior::Indicator), 7) << stored;
    EXPECT_EQ(get_clicks(Behavior::Scroll), 6) << stored;
  }
}

// Goes through the real parameter path: the user parameters clear the cached values while they are saved
TEST_F(ButtonBindingsTest, links_survive_the_system_parameters_cycle)
{
  ASSERT_EQ(set_binding(Behavior::Favorite, 9), SetResult::Success);
  behavior::write_parameters();

  reset_to_defaults();
  bsp::filesystem::user::clear_cached();

  behavior::read_parameters();
  EXPECT_EQ(get_clicks(Behavior::Favorite), 9);

  // clearing the user parameters restores the defaults
  EXPECT_TRUE(bsp::filesystem::user::format());
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);
}

// The click and the hold of a behavior follow its binding, and leave the previous clicks free
TEST_F(ButtonBindingsTest, button_handlers_follow_the_binding)
{
  EXPECT_TRUE(user::button_clicked_default(4));
  EXPECT_TRUE(user::button_hold_default(4, true, 0));
  EXPECT_FALSE(user::button_clicked_default(8));
  EXPECT_FALSE(user::button_hold_default(8, true, 0));

  ASSERT_EQ(set_binding(Behavior::Favorite, 8), SetResult::Success);

  EXPECT_TRUE(user::button_clicked_default(8));
  EXPECT_TRUE(user::button_hold_default(8, true, 0));
  EXPECT_FALSE(user::button_clicked_default(4));
  EXPECT_FALSE(user::button_hold_default(4, true, 0));
}

TEST_F(ButtonBindingsTest, only_the_ramp_follows_the_binding_and_3_clicks_stays_the_group_navigation)
{
  EXPECT_TRUE(user::button_clicked_default(3));
  EXPECT_FALSE(user::button_clicked_default(8));
  EXPECT_FALSE(user::button_hold_default(8, true, 0));

  ASSERT_EQ(set_binding(Behavior::CustomRamp, 8), SetResult::Success);

  EXPECT_TRUE(user::button_hold_default(8, true, 0));
  EXPECT_FALSE(user::button_hold_default(3, true, 0));

  // the click did not move
  EXPECT_TRUE(user::button_clicked_default(3));
  EXPECT_FALSE(user::button_clicked_default(8));
}

TEST_F(ButtonBindingsTest, only_hold_behaviors_can_use_3_clicks)
{
  ASSERT_EQ(set_binding(Behavior::CustomRamp, 8), SetResult::Success);
  EXPECT_EQ(get_hold_behavior(3), Behavior::None);

  for (const auto behavior: {Behavior::Favorite, Behavior::QuickNavigation, Behavior::Indicator})
  {
    const uint8_t clicks = get_clicks(behavior);
    EXPECT_EQ(set_binding(behavior, 3), SetResult::ReservedClicks);
    EXPECT_EQ(get_clicks(behavior), clicks);
  }

  EXPECT_EQ(set_binding(Behavior::Sunset, 3), SetResult::Success);
  EXPECT_EQ(get_hold_behavior(3), Behavior::Sunset);
  EXPECT_EQ(set_binding(Behavior::CustomRamp, 3), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Sunset), 8);
}

TEST_F(ButtonBindingsTest, swap_refused_if_the_other_behavior_cannot_use_3_clicks)
{
  EXPECT_EQ(set_binding(Behavior::CustomRamp, 4), SetResult::SwapNotPossible);
  EXPECT_EQ(get_clicks(Behavior::CustomRamp), 3);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);

  // a hold only behavior can swap
  EXPECT_EQ(set_binding(Behavior::CustomRamp, 5), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::CustomRamp), 5);
  EXPECT_EQ(get_clicks(Behavior::Sunset), 3);
}

TEST_F(ButtonBindingsTest, nav_and_scroll_move_separately)
{
  EXPECT_FALSE(user::button_hold_default(8, true, 0));
  EXPECT_FALSE(user::button_clicked_default(8));

  // the scroll moves alone
  ASSERT_EQ(set_binding(Behavior::Scroll, 8), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 6);
  EXPECT_TRUE(user::button_hold_default(8, true, 0));
  EXPECT_FALSE(user::button_hold_default(6, true, 0));
  EXPECT_TRUE(user::button_clicked_default(6));
  EXPECT_FALSE(user::button_clicked_default(8));

  // the click only nav can join it, without a swap
  ASSERT_EQ(set_binding(Behavior::QuickNavigation, 8), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Scroll), 8);
  EXPECT_TRUE(user::button_clicked_default(8));
  EXPECT_TRUE(user::button_hold_default(8, true, 0));
  EXPECT_FALSE(user::button_clicked_default(6));
}

TEST_F(ButtonBindingsTest, hold_only_and_click_only_behaviors_share_clicks_and_survive_a_save)
{
  ASSERT_EQ(set_binding(Behavior::Sunset, 8), SetResult::Success);
  ASSERT_EQ(set_binding(Behavior::QuickNavigation, 8), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Sunset), 8);
  EXPECT_EQ(get_click_behavior(8), Behavior::QuickNavigation);
  EXPECT_EQ(get_hold_behavior(8), Behavior::Sunset);

  write_to_memory();
  reset_to_defaults();
  load_from_memory();
  EXPECT_EQ(get_click_behavior(8), Behavior::QuickNavigation);
  EXPECT_EQ(get_hold_behavior(8), Behavior::Sunset);
}

TEST_F(ButtonBindingsTest, swap_refused_if_it_collides_with_what_stayed)
{
  // favorite would go to 6, where its hold collides with the scroll
  EXPECT_EQ(set_binding(Behavior::QuickNavigation, 4), SetResult::SwapNotPossible);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 6);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 4);
  EXPECT_EQ(get_clicks(Behavior::Scroll), 6);

  // free the hold first, the swap is now possible
  ASSERT_EQ(set_binding(Behavior::Scroll, 8), SetResult::Success);
  EXPECT_EQ(set_binding(Behavior::QuickNavigation, 4), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::Favorite), 6);
}

TEST_F(ButtonBindingsTest, swapped_handlers_follow_the_binding)
{
  ASSERT_EQ(set_binding(Behavior::QuickNavigation, 8), SetResult::Success);
  EXPECT_FALSE(user::button_clicked_default(6));
  EXPECT_TRUE(user::button_clicked_default(8));

  // nav goes back to the clicks freed by favorite
  ASSERT_EQ(set_binding(Behavior::Favorite, 8), SetResult::Success);
  EXPECT_EQ(get_clicks(Behavior::QuickNavigation), 4);
  EXPECT_TRUE(user::button_clicked_default(4));
  EXPECT_TRUE(user::button_clicked_default(8));
  EXPECT_FALSE(user::button_clicked_default(6));
}

TEST_F(ButtonBindingsTest, indicator_input_follows_the_binding)
{
  const auto level_after_clicks = [](uint8_t clicks) {
    EXPECT_TRUE(inputs::add_button_click_event(clicks));
    inputs::loop();
    return logic::indicator::get_brightness_level();
  };
  const auto level_after_hold = [](uint8_t clicks) {
    // the indicator updates at most once per second while held: be sure this hold is allowed to update it
    ::simulator::time_mocks::reset(hal::time_ms() + 2000);

    EXPECT_TRUE(inputs::add_button_press_event(clicks, component::button::HOLD_BUTTON_MIN_MS + 100, false));
    inputs::loop();
    EXPECT_TRUE(inputs::add_button_press_event(clicks, 0, true));
    inputs::loop();
    return logic::indicator::get_brightness_level();
  };

  const uint8_t initial = logic::indicator::get_brightness_level();
  EXPECT_EQ(level_after_clicks(8), initial);

  const uint8_t afterDefault = level_after_clicks(7);
  EXPECT_NE(afterDefault, initial);

  ASSERT_EQ(set_binding(Behavior::Indicator, 8), SetResult::Success);
  EXPECT_EQ(level_after_clicks(7), afterDefault);
  const uint8_t afterClick = level_after_clicks(8);
  EXPECT_NE(afterClick, afterDefault);

  // the hold action follows the binding as well
  EXPECT_EQ(level_after_hold(7), afterClick);
  EXPECT_NE(level_after_hold(8), afterClick);
}

} // namespace button_bindings
} // namespace logic
} // namespace lampda
