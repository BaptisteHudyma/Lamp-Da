#include <gtest/gtest.h>

#include "src/system/common/user_commands.h"
#include "src/system/component/time_handling.h"

namespace lampda {
namespace common {

class UserCommandTest : public ::testing::Test
{
protected:
  void SetUp() override {}
};

// Helper to access protected constructor for invalid state testing
class TestableUserCommand : public UserCommand
{
public:
  TestableUserCommand() : UserCommand() {}
};

TEST_F(UserCommandTest, SetRampCommand)
{
  uint8_t ramp = 50;
  auto cmd = UserCommand::make_set_ramp_command(ramp);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetUserRamp);

  uint8_t parsed_ramp = 0;
  EXPECT_TRUE(cmd.parse_ramp(parsed_ramp));
  EXPECT_EQ(parsed_ramp, ramp);
}

TEST_F(UserCommandTest, BrightnessCommand)
{
  brightness_t bright = 128;
  auto cmd = UserCommand::make_brightness_command(bright);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::Brightness);

  brightness_t parsed_bright = 0;
  EXPECT_TRUE(cmd.parse_brightness(parsed_bright));
  EXPECT_EQ(parsed_bright, bright);
}

TEST_F(UserCommandTest, SetModeCommand)
{
  uint8_t groupId = 1;
  uint8_t modeId = 3;
  auto cmd = UserCommand::make_set_mode_command(groupId, modeId);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetMode);

  uint8_t parsed_gid = 0, parsed_mid = 0;
  EXPECT_TRUE(cmd.parse_set_mode(parsed_gid, parsed_mid));
  EXPECT_EQ(parsed_gid, groupId);
  EXPECT_EQ(parsed_mid, modeId);
}

TEST_F(UserCommandTest, TurnOnOffCommand)
{
  auto cmd_on = UserCommand::make_turn_onoff_command(true);
  EXPECT_EQ(cmd_on.get_type(), UserCommand::Type::OnOff);
  bool val = false;
  EXPECT_TRUE(cmd_on.parse_turn_onoff(val));
  EXPECT_TRUE(val);

  auto cmd_off = UserCommand::make_turn_onoff_command(false);
  val = true;
  EXPECT_TRUE(cmd_off.parse_turn_onoff(val));
  EXPECT_FALSE(val);
}

TEST_F(UserCommandTest, SetRealTimeCommand)
{
  component::time::RealTime time {0, 2, 30, 45};
  auto cmd = UserCommand::make_set_real_time_command(time);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetRealTime);

  component::time::RealTime parsed_time;
  EXPECT_TRUE(cmd.parse_set_real_time_command(parsed_time));
}

TEST_F(UserCommandTest, SetSunsetToTimeCommand)
{
  component::time::RealTime time {0, 20, 0, 0};
  auto cmd = UserCommand::make_set_sunset_to_time_command(time);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetSunsetToTime);

  component::time::RealTime parsed_time;
  EXPECT_TRUE(cmd.parse_set_sunset_to_time_command(parsed_time));
}

TEST_F(UserCommandTest, SetBleCustomColorModeCommand)
{
  uint8_t r = 255, g = 128, b = 64;
  auto cmd = UserCommand::make_set_ble_custom_color_mode_command(r, g, b);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetBleCustomColorMode);

  uint32_t color = 0;
  EXPECT_TRUE(cmd.parse_set_ble_custom_color_mode_command(color));
  EXPECT_NE(color, 0); // Ensure color data was stored
}

TEST_F(UserCommandTest, SetBleModeCommand)
{
  uint8_t idx = 5;
  auto cmd = UserCommand::make_set_ble_mode_command(idx);
  EXPECT_EQ(cmd.get_type(), UserCommand::Type::SetBleMode);

  uint8_t parsed_idx = 0;
  EXPECT_TRUE(cmd.parse_set_ble_mode_command(parsed_idx));
  EXPECT_EQ(parsed_idx, idx);
}

TEST_F(UserCommandTest, InvalidCommandParsingReturnsFalse)
{
  TestableUserCommand invalid_cmd;
  EXPECT_EQ(invalid_cmd.get_type(), UserCommand::Type::Invalid);

  uint8_t ramp = 0;
  EXPECT_FALSE(invalid_cmd.parse_ramp(ramp));

  brightness_t bright = 0;
  EXPECT_FALSE(invalid_cmd.parse_brightness(bright));

  uint8_t gid = 0, mid = 0;
  EXPECT_FALSE(invalid_cmd.parse_set_mode(gid, mid));

  bool on = false;
  EXPECT_FALSE(invalid_cmd.parse_turn_onoff(on));

  component::time::RealTime time;
  EXPECT_FALSE(invalid_cmd.parse_set_real_time_command(time));
  EXPECT_FALSE(invalid_cmd.parse_set_sunset_to_time_command(time));

  uint32_t color = 0;
  EXPECT_FALSE(invalid_cmd.parse_set_ble_custom_color_mode_command(color));

  uint8_t idx = 0;
  EXPECT_FALSE(invalid_cmd.parse_set_ble_mode_command(idx));
}

TEST_F(UserCommandTest, BrightnessEdgeCases)
{
  // Zero brightness
  auto cmd_zero = UserCommand::make_brightness_command(0);
  brightness_t val_zero = 0;
  EXPECT_TRUE(cmd_zero.parse_brightness(val_zero));
  EXPECT_EQ(val_zero, 0);

  // Maximum valid brightness (1024)
  auto cmd_max = UserCommand::make_brightness_command(1024);
  brightness_t val_max = 0;
  EXPECT_TRUE(cmd_max.parse_brightness(val_max));
  EXPECT_EQ(val_max, 1024);

  // Overflow brightness (1025) -> should be rejected by parse
  // Note: make_brightness_command takes uint16_t, so we can pass 1025.
  // The internal storage is 2 bytes. 1025 is 0x0401.
  // Data[0]=1, Data[1]=4.
  // Reconstructed: (4 << 8) | 1 = 1025.
  // 1025 > 1024 -> returns false.
  auto cmd_overflow = UserCommand::make_brightness_command(1025);
  brightness_t val_overflow = 0;
  EXPECT_FALSE(cmd_overflow.parse_brightness(val_overflow));
}

TEST_F(UserCommandTest, RealTimeValidity)
{
  // Valid time
  component::time::RealTime valid_time {3, 12, 30, 45};
  auto cmd_valid = UserCommand::make_set_real_time_command(valid_time);
  component::time::RealTime parsed_valid;
  EXPECT_TRUE(cmd_valid.parse_set_real_time_command(parsed_valid));
  EXPECT_EQ(parsed_valid.hour, 12);
  EXPECT_EQ(parsed_valid.minutes, 30);
  EXPECT_EQ(parsed_valid.seconds, 45);
  EXPECT_EQ(parsed_valid.dayOfTheWeek, 3);

  // Invalid time: Hour 24
  component::time::RealTime invalid_hour {0, 24, 0, 0};
  auto cmd_invalid_hour = UserCommand::make_set_real_time_command(invalid_hour);
  component::time::RealTime parsed_invalid_hour;
  EXPECT_FALSE(cmd_invalid_hour.parse_set_real_time_command(parsed_invalid_hour));

  // Invalid time: Minutes 60
  component::time::RealTime invalid_min {0, 0, 60, 0};
  auto cmd_invalid_min = UserCommand::make_set_real_time_command(invalid_min);
  component::time::RealTime parsed_invalid_min;
  EXPECT_FALSE(cmd_invalid_min.parse_set_real_time_command(parsed_invalid_min));

  // Invalid time: Seconds 60
  component::time::RealTime invalid_sec {0, 0, 0, 60};
  auto cmd_invalid_sec = UserCommand::make_set_real_time_command(invalid_sec);
  component::time::RealTime parsed_invalid_sec;
  EXPECT_FALSE(cmd_invalid_sec.parse_set_real_time_command(parsed_invalid_sec));

  // Invalid time: Day 7 (valid is 0-6)
  component::time::RealTime invalid_day {7, 0, 0, 0};
  auto cmd_invalid_day = UserCommand::make_set_real_time_command(invalid_day);
  component::time::RealTime parsed_invalid_day;
  EXPECT_FALSE(cmd_invalid_day.parse_set_real_time_command(parsed_invalid_day));
}

TEST_F(UserCommandTest, CommandCopyPreservesData)
{
  auto original = UserCommand::make_set_mode_command(2, 5);

  // Copy constructor
  auto copy = original;
  EXPECT_EQ(copy.get_type(), original.get_type());

  uint8_t gid = 0, mid = 0;
  EXPECT_TRUE(copy.parse_set_mode(gid, mid));
  EXPECT_EQ(gid, 2);
  EXPECT_EQ(mid, 5);

  // Assignment operator
  auto assigned = original;
  EXPECT_EQ(assigned.get_type(), original.get_type());
  EXPECT_TRUE(assigned.parse_set_mode(gid, mid));
  EXPECT_EQ(gid, 2);
  EXPECT_EQ(mid, 5);
}

TEST_F(UserCommandTest, SunsetTimeEdgeCases)
{
  component::time::RealTime valid_sunset {0, 18, 30, 0};
  auto cmd = UserCommand::make_set_sunset_to_time_command(valid_sunset);
  component::time::RealTime parsed;
  EXPECT_TRUE(cmd.parse_set_sunset_to_time_command(parsed));
  EXPECT_EQ(parsed.hour, 18);
  EXPECT_EQ(parsed.minutes, 30);

  component::time::RealTime invalid_sunset {25, 0, 0, 0};
  auto cmd_invalid = UserCommand::make_set_sunset_to_time_command(invalid_sunset);
  component::time::RealTime parsed_invalid;
  EXPECT_FALSE(cmd_invalid.parse_set_sunset_to_time_command(parsed_invalid));
}

} // namespace common
} // namespace lampda
