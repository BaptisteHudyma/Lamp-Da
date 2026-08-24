
#include "cmd_parser.h"

#include <cerrno>
#include <cstdlib>

namespace lampda {
namespace utils {
namespace cli {

inline bool isCommandSeparator(const char character) { return character == ' ' || character == '\t'; }

const char* ParsedCommand::name() const { return buffer.data() + commandOffset; }

const char* ParsedCommand::argument(const size_t index) const
{
  if (index >= argumentCount)
    return nullptr;

  return buffer.data() + argumentOffsets[index];
}

ParsedCommand ParsedCommand::shift_to_first_parameter() const
{
  const bool hasArguments = this->argumentCount > 0;

  ParsedCommand subCommand;
  subCommand.argumentCount = hasArguments ? this->argumentCount - 1 : 0;
  for (size_t i = 1; i < ParsedCommand::maxArgumentCount; i++)
    subCommand.argumentOffsets[i - 1] = this->argumentOffsets[i];
  subCommand.commandOffset = hasArguments ? this->argumentOffsets[0] : (bsp::text_in::Inputs::maxCommandSize - 1);
  subCommand.buffer = this->buffer;
  return subCommand;
}

namespace argument {

/// Generic parse function for unsigned integers
bool parse_uint(const ParsedCommand& command, const size_t index, unsigned long& value)
{
  const char* text = command.argument(index);

  if (text == nullptr)
    return false;

  char* end = nullptr;
  errno = 0;

  const unsigned long tmp_value = std::strtoul(text, &end, 0);

  if (errno != 0 || end == text || *end != '\0')
  {
    return false;
  }
  value = tmp_value;
  return true;
}

/// Parse an argument as a 8 bit unsigned number
bool parse_uint8(const ParsedCommand& command, const size_t index, uint8_t& value)
{
  unsigned long argument;
  const bool isValid = parse_uint(command, index, argument);
  if (!isValid)
    return false;
  if (argument > UINT8_MAX)
    return false;

  value = static_cast<uint8_t>(argument);
  return true;
}

/// Parse an argument as a 16 bit unsigned number
bool parse_uint16(const ParsedCommand& command, const size_t index, uint16_t& value)
{
  unsigned long argument;
  const bool isValid = parse_uint(command, index, argument);
  if (!isValid)
    return false;
  if (argument > UINT16_MAX)
    return false;

  value = argument;
  return true;
}
} // namespace argument

ParsedCommand parseCommand(const bsp::text_in::Inputs::Command& input)
{
  ParsedCommand result;
  // copy str
  result.buffer = input;

  size_t position = 0;

  // skip all first blank char define in isCommandSeparator
  while (position < result.buffer.size() && isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  result.commandOffset = static_cast<uint8_t>(position);

  // looking for the end of the command
  while (position < result.buffer.size() && result.buffer[position] != '\0' &&
         !isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  // put a \0 at the end (useful for the hash command and some print)
  if (position < result.buffer.size() && result.buffer[position] != '\0')
  {
    result.buffer[position] = '\0';
    ++position;
  }

  // now, parse every argument
  result.argumentOffsets.fill(0);
  while (position < result.buffer.size() && result.argumentCount < ParsedCommand::maxArgumentCount)
  {
    // while it's a blank char
    while (position < result.buffer.size() && isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // terminated condition of the while (end of the string)
    if (position >= result.buffer.size() || result.buffer[position] == '\0')
    {
      break;
    }

    // save the offset
    result.argumentOffsets[result.argumentCount++] = static_cast<uint8_t>(position);

    // looking for the end of the argument
    while (position < result.buffer.size() && result.buffer[position] != '\0' &&
           !isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // put a \0 at the end
    if (position < result.buffer.size() && result.buffer[position] != '\0')
    {
      result.buffer[position] = '\0';
      ++position;
    }
  }

  return result;
}

} // namespace cli
} // namespace utils
} // namespace lampda
