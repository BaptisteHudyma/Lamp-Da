/*! \file command_line_interface.h
    \brief User command line interface logic.
*/

namespace lampda {
namespace logic {
namespace cli {

/// Call once on system start
void setup();
/// Handle the user command line inputs.
void handleSerialEvents();

} // namespace cli
} // namespace logic
} // namespace lampda
