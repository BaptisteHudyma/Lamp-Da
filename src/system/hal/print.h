/*! \file print.h
    \brief Interface for the platform specific debug and prints.
*/

#ifndef HAL_PRINT_H
#define HAL_PRINT_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <stdarg.h>

#include "src/system/utils/print.h"

namespace lampda {
namespace hal {
/// Print and debug function to serial port

/**
 * \brief call once at program start
 */
extern void init_prints();

/**
 * \brief read external inputs (may take some time)
 */

struct Inputs
{
  static constexpr size_t maxCommandSize = 63;
  typedef std::array<char, maxCommandSize> Command;

  static constexpr uint8_t maxCommands = 2;
  std::array<Command, maxCommands> commandList;
  uint8_t commandCount = 0;
};

extern Inputs read_inputs();

} // namespace hal
} // namespace lampda

#endif
