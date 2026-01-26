#ifndef PLATFORM_PRINT_H
#define PLATFORM_PRINT_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <stdarg.h>

#include "src/system/utils/print.h"

/**
 * \brief call once at program start
 */
extern void init_prints();

/**
 * \brief read external inputs (may take some time)
 */

struct Inputs
{
  static constexpr size_t maxCommandSize = 32;
  typedef std::array<char, maxCommandSize> Command;

  static constexpr size_t maxCommands = 2;
  std::array<Command, maxCommands> commandList;
  uint8_t commandCount = 0;
};

extern Inputs read_inputs();

#endif
