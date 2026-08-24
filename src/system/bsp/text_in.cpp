#include "text_in.h"

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/serial.h"

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
serial_backend_ops_t ble_ueart_ops = {.is_activated = hal::bluetooth::serial::is_activated,
                                      .read_char = hal::bluetooth::serial::read,
                                      .is_ready = hal::bluetooth::serial::is_available};

namespace __private {

// only keep the chars inside a certain ascii range
bool is_ignore_char(char c) { return c < 32; }

void generic_parse_serial(const serial_backend_ops_t* ops, Inputs& inputs)
{
  if (not ops->is_activated())
    return;

  if (ops->is_ready())
  {
    uint8_t charRead = 0;

    // read available serial data
    do
    {
      // get the new byte:
      const char inChar = ops->read_char();
      // if the incoming character is a newline, finish parsing
      if (inChar == '\n')
      {
        // do not add empty strings and null terminated only strings
        if (charRead != 0)
        {
          // add null termination if needed
          if (charRead < Inputs::maxCommandSize)
          {
            if (inputs.commandList[inputs.commandCount][charRead] != '\0')
              inputs.commandList[inputs.commandCount][charRead] = '\0';
          }
          else
          {
            inputs.commandList[inputs.commandCount][Inputs::maxCommandSize - 1] = '\0';
          }
          inputs.commandCount += 1;
        }
        else
        {
          for (size_t i = 0; i < Inputs::maxCommandSize; i++)
            inputs.commandList[inputs.commandCount][i] = '\0';
        }

        charRead = 0;
      }
      else if (charRead < Inputs::maxCommandSize)
      {
        // add it to the inputString:
        if (not is_ignore_char(inChar))
        {
          inputs.commandList[inputs.commandCount][charRead] = inChar;
          charRead += 1;
        }
      }
    } while (ops->is_ready() && inputs.commandCount < Inputs::maxCommands);
  }
}

} // namespace __private

Inputs inputs;
Inputs read_inputs()
{
  inputs.reset();
  __private::generic_parse_serial(&serial_uart_ops, inputs);
  __private::generic_parse_serial(&ble_ueart_ops, inputs);
  return inputs;
}

} // namespace text_in
} // namespace bsp
} // namespace lampda
