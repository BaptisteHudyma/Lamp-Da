#include "src/system/utils/cmd_parser.h"
#include "src/system/bsp/text_in.h"

#include <gtest/gtest.h>

namespace lampda::utils::cli {

// ============ parseCommand tests ============

bsp::text_in::Inputs::Command create_command(const std::string& text)
{
  bsp::text_in::Inputs::Command input {};
  std::copy(text.cbegin(), text.cend(), input.begin());
  return input;
}

TEST(cmd_parser, empty_command)
{
  bsp::text_in::Inputs::Command input {};
  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 0);
  EXPECT_STREQ(result.name(), "");
  EXPECT_EQ(result.argument(0), nullptr);
}

TEST(cmd_parser, command_without_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("hello");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 0);
  EXPECT_STREQ(result.name(), "hello");
}

TEST(cmd_parser, command_with_single_argument)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg1");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 1);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg1");
  EXPECT_EQ(result.argument(1), nullptr);
}

TEST(cmd_parser, command_with_multiple_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg1 arg2 arg3");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 3);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg1");
  EXPECT_STREQ(result.argument(1), "arg2");
  EXPECT_STREQ(result.argument(2), "arg3");
  EXPECT_EQ(result.argument(3), nullptr);
}

TEST(cmd_parser, command_with_many_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("a b c d e f g h");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 7);
  EXPECT_STREQ(result.name(), "a");
  EXPECT_STREQ(result.argument(0), "b");
  EXPECT_STREQ(result.argument(1), "c");
  EXPECT_STREQ(result.argument(2), "d");
  EXPECT_STREQ(result.argument(3), "e");
  EXPECT_STREQ(result.argument(4), "f");
  EXPECT_STREQ(result.argument(5), "g");
  EXPECT_STREQ(result.argument(6), "h");
  EXPECT_EQ(result.argument(7), nullptr);
}

TEST(cmd_parser, leading_whitespace)
{
  const bsp::text_in::Inputs::Command input = create_command("   cmd arg");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 1);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg");
}

TEST(cmd_parser, trailing_whitespace)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg   ");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 1);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg");
}

TEST(cmd_parser, multiple_spaces_between_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd  arg1   arg2");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 2);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg1");
  EXPECT_STREQ(result.argument(1), "arg2");
}

TEST(cmd_parser, tab_separator)
{
  bsp::text_in::Inputs::Command input {};
  input[0] = 'c';
  input[1] = 'm';
  input[2] = 'd';
  input[3] = '\t';
  input[4] = 'a';
  input[5] = 'r';
  input[6] = 'g';
  input[7] = '\t';
  input[8] = '1';
  input[8] = '\0';

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 1);
  EXPECT_STREQ(result.name(), "cmd");
  EXPECT_STREQ(result.argument(0), "arg");
}

TEST(cmd_parser, max_arguments_limit)
{
  // 9 arguments should result in only 8 being parsed
  const bsp::text_in::Inputs::Command input = create_command("c a b c d e f g h i");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 8);
  EXPECT_STREQ(result.name(), "c");
  EXPECT_STREQ(result.argument(0), "a");
  EXPECT_STREQ(result.argument(7), "h");
  EXPECT_EQ(result.argument(8), nullptr);
}

TEST(cmd_parser, single_character_command)
{
  const bsp::text_in::Inputs::Command input = create_command("x");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 0);
  EXPECT_STREQ(result.name(), "x");
}

TEST(cmd_parser, argument_with_no_space_after_command)
{
  // When there's no space after command, it should be part of the command
  const bsp::text_in::Inputs::Command input = create_command("cmd");

  auto result = parseCommand(input);

  EXPECT_EQ(result.argumentCount, 0);
  EXPECT_STREQ(result.name(), "cmd");
}

// ============ ParsedCommand::name() tests ============

TEST(cmd_parser, name_empty_buffer)
{
  bsp::text_in::Inputs::Command input {};
  auto result = parseCommand(input);

  EXPECT_STREQ(result.name(), "");
}

TEST(cmd_parser, name_with_trailing_null)
{
  const bsp::text_in::Inputs::Command input = create_command("test");

  auto parsed = parseCommand(input);

  EXPECT_STREQ(parsed.name(), "test");
}

// ============ ParsedCommand::argument() tests ============

TEST(cmd_parser, argument_valid_index)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg0 arg1");

  auto parsed = parseCommand(input);

  EXPECT_STREQ(parsed.argument(0), "arg0");
  EXPECT_STREQ(parsed.argument(1), "arg1");
}

TEST(cmd_parser, argument_out_of_range)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg0");

  auto parsed = parseCommand(input);

  EXPECT_STREQ(parsed.argument(0), "arg0");
  EXPECT_EQ(parsed.argument(1), nullptr);
  EXPECT_EQ(parsed.argument(2), nullptr);
  EXPECT_EQ(parsed.argument(100), nullptr);
}

// ============ ParsedCommand::shift_to_first_parameter() tests ============

TEST(cmd_parser, shift_with_one_argument)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg1");

  auto parsed = parseCommand(input);
  auto shifted = parsed.shift_to_first_parameter();

  EXPECT_EQ(shifted.argumentCount, 0);
  EXPECT_STREQ(shifted.name(), "arg1");
}

TEST(cmd_parser, shift_with_multiple_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd arg1 arg2 arg3");

  auto parsed = parseCommand(input);
  auto shifted = parsed.shift_to_first_parameter();

  EXPECT_EQ(shifted.argumentCount, 2);
  EXPECT_STREQ(shifted.name(), "arg1");
  EXPECT_STREQ(shifted.argument(0), "arg2");
  EXPECT_STREQ(shifted.argument(1), "arg3");
}

TEST(cmd_parser, shift_with_no_arguments)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd");

  auto parsed = parseCommand(input);
  auto shifted = parsed.shift_to_first_parameter();

  EXPECT_EQ(shifted.argumentCount, 0);
  EXPECT_STREQ(shifted.name(), "");
}

// ============ argument::parse_uint tests ============

TEST(cmd_parser, parse_uint8_valid)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 123");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 123);
}

TEST(cmd_parser, parse_uint8_zero)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 0");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 0);
}

TEST(cmd_parser, parse_uint8_max)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 255");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 255);
}

TEST(cmd_parser, parse_uint8_overflow)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 256");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_FALSE(argument::parse_uint8(parsed, 0, value));
}

TEST(cmd_parser, parse_uint8_negative)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd -1");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_FALSE(argument::parse_uint8(parsed, 0, value));
}

TEST(cmd_parser, parse_uint8_non_numeric)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd abc");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_FALSE(argument::parse_uint8(parsed, 0, value));
}

TEST(cmd_parser, parse_uint8_partial_numeric)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 123abc");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_FALSE(argument::parse_uint8(parsed, 0, value));
}

TEST(cmd_parser, parse_uint8_missing_argument)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_FALSE(argument::parse_uint8(parsed, 0, value));
}

TEST(cmd_parser, parse_uint16_valid)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 12345");
  auto parsed = parseCommand(input);

  uint16_t value = 0;
  EXPECT_TRUE(argument::parse_uint16(parsed, 0, value));
  EXPECT_EQ(value, 12345);
}

TEST(cmd_parser, parse_uint16_max)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 65535");
  auto parsed = parseCommand(input);

  uint16_t value = 0;
  EXPECT_TRUE(argument::parse_uint16(parsed, 0, value));
  EXPECT_EQ(value, 65535);
}

TEST(cmd_parser, parse_uint16_overflow)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 65536");
  auto parsed = parseCommand(input);

  uint16_t value = 0;
  EXPECT_FALSE(argument::parse_uint16(parsed, 0, value));
}

TEST(cmd_parser, parse_uint16_negative)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd -1");
  auto parsed = parseCommand(input);

  uint16_t value = 0;
  EXPECT_FALSE(argument::parse_uint16(parsed, 0, value));
}

TEST(cmd_parser, parse_uint16_non_numeric)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd abc");
  auto parsed = parseCommand(input);

  uint16_t value = 0;
  EXPECT_FALSE(argument::parse_uint16(parsed, 0, value));
}

// ============ Hex, Octal, Decimal parsing ============

TEST(cmd_parser, parse_uint_hex)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 0xFF");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 255);
}

TEST(cmd_parser, parse_uint_octal)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 010");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 8);
}

TEST(cmd_parser, parse_uint_binary_prefix)
{
  const bsp::text_in::Inputs::Command input = create_command("cmd 0b1010");
  auto parsed = parseCommand(input);

  uint8_t value = 0;
  EXPECT_TRUE(argument::parse_uint8(parsed, 0, value));
  EXPECT_EQ(value, 10);
}

} // namespace lampda::utils::cli
