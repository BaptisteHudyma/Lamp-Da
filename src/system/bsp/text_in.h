/*! \file text_in.h
    \brief Handle user text inputs.
*/

#ifndef BSP_TEXT_IN_H
#define BSP_TEXT_IN_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace lampda {
namespace bsp {
/// Handle the system user input
namespace text_in {

/**
 * \brief read external inputs (may take some time)
 */

struct Inputs
{
  static constexpr size_t maxCommandSize = 63;
  using Command = std::array<char, maxCommandSize>;

  static constexpr uint8_t maxCommands = 2;
  std::array<Command, maxCommands> commandList;
  uint8_t commandCount = 0;
};

extern Inputs read_inputs();

} // namespace text_in
} // namespace bsp
} // namespace lampda

#endif
