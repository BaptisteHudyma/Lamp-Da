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

struct CommandHandle
{
  enum Type
  {
    SetUserRamp, ///< set the user ramp value
    Brightness,  ///< set brightness
    SetMode,     ///< set mode index
  };
  static constexpr uint8_t maxDataSize = 8;

  Type type;                             ///< request type
  uint8_t dataCnt;                       ///< used data count
  std::array<uint8_t, maxDataSize> data; ///< actual stored data
};

/// Call once on system start
void setup();
/// Handle the user command line inputs.
void handleSerialEvents();

} // namespace cli
} // namespace logic
} // namespace lampda

#endif
