/*! \file cmd_parser.h
    \brief Command argument parser logic
*/
#ifndef UTILS_CMD_PARSER_H
#define UTILS_CMD_PARSER_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "src/system/bsp/text_in.h"

namespace lampda {
namespace utils {
namespace cli {

struct ParsedCommand
{
  static constexpr uint8_t maxArgumentCount = 8;

  bsp::text_in::Inputs::Command buffer {};

  uint8_t commandOffset = 0;
  std::array<uint8_t, maxArgumentCount> argumentOffsets {};
  uint8_t argumentCount = 0;

  /// Return the name of the command
  const char* name() const;

  /// Return the argument at the desired index, or nullptr if it do not exist
  const char* argument(const size_t index) const;

  /// Create a new command shifted by one argument.
  ParsedCommand shift_to_first_parameter() const;
};

/// Tools to parse text arguments
namespace argument {

/// Parse an argument as a 8 bit unsigned number
bool parse_uint8(const ParsedCommand& command, const size_t index, uint8_t& value);

/// Parse an argument as a 16 bit unsigned number
bool parse_uint16(const ParsedCommand& command, const size_t index, uint16_t& value);

} // namespace argument

/**
 * \brief Parse a command to extract its name and arguments
 */
ParsedCommand parseCommand(const bsp::text_in::Inputs::Command& input);

} // namespace cli
} // namespace utils
} // namespace lampda

#endif
