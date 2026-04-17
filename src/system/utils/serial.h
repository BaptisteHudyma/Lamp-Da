/*! \file serial.h
    \brief User CommandLineInterface.
*/

namespace lampda {
namespace utils {
namespace serial {

/// Call once on system start
void setup();
/// Handle the user command line inputs.
void handleSerialEvents();

} // namespace serial
} // namespace utils
} // namespace lampda
