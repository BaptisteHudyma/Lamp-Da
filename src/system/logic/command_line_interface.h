/*! \file command_line_interface.h
    \brief User command line interface logic.
*/
#ifndef COMMAND_LINE_INTERFACE
#define COMMAND_LINE_INTERFACE

#include <array>
#include <cstdint>
namespace lampda {
namespace logic {
/// Handle the serial command line interface
namespace cli {

/// Call once on system start
void setup();
/// Handle the user command line inputs.
void handleSerialEvents();

} // namespace cli
} // namespace logic
} // namespace lampda

#endif
