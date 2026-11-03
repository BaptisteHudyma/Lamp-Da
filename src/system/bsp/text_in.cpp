#include "text_in.h"

#include <cstring>

#include "src/system/hal/serial.h"

#include "src/system/bsp/ble.h"

namespace lampda {
namespace bsp {
namespace text_in {

typedef struct
{
  bool (*is_activated)();
  char (*read_char)();
  bool (*is_ready)();
} serial_backend_ops_t;

/// local uart serial handle
serial_backend_ops_t serial_uart_ops = {.is_activated = hal::serial::is_activated,
                                        .read_char = hal::serial::read,
                                        .is_ready = hal::serial::is_available};
serial_backend_ops_t ble_ueart_ops = {.is_activated = bsp::ble::serial::is_activated,
                                      .read_char = bsp::ble::serial::read,
                                      .is_ready = bsp::ble::serial::is_available};

namespace __private {

// only keep the chars inside a certain ascii range
bool is_ignore_char(char c) { return c < 32; }

/// Persistent line assembler: one instance per backend, statically allocated.
struct LineAssembler
{
  char buffer[Inputs::maxCommandSize];
  size_t length = 0;
  bool overflow = false; // line too long: drop everything until next '\n'

  void reset()
  {
    length = 0;
    overflow = false;
  }
};

LineAssembler uart_line;
LineAssembler ble_line;

void generic_parse_serial(const serial_backend_ops_t* ops, LineAssembler& line, Inputs& inputs)
{
  if (not ops->is_activated())
  {
    // connection lost: do not keep a stale partial command
    line.reset();
    return;
  }

  while (inputs.commandCount < Inputs::maxCommands && ops->is_ready())
  {
    const char inChar = ops->read_char();

    if (inChar == '\n')
    {
      // complete line received: publish it only if valid and non empty
      if (not line.overflow && line.length > 0)
      {
        char* dst = inputs.commandList[inputs.commandCount].data();
        memcpy(dst, line.buffer, line.length);
        dst[line.length] = '\0';
        inputs.commandCount += 1;
      }
      line.reset();
    }
    else if (not is_ignore_char(inChar) && not line.overflow)
    {
      // keep room for the null terminator
      if (line.length < Inputs::maxCommandSize - 1)
        line.buffer[line.length++] = inChar;
      else
        line.overflow = true; // too long: reject the whole command
    }
  }
}

} // namespace __private

Inputs inputs;
Inputs read_inputs()
{
  inputs.reset();
  __private::generic_parse_serial(&serial_uart_ops, __private::uart_line, inputs);
  __private::generic_parse_serial(&ble_ueart_ops, __private::ble_line, inputs);
  return inputs;
}

} // namespace text_in
} // namespace bsp
} // namespace lampda
