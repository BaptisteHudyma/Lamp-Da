#include "text_in.h"

#include "src/system/hal/serial.h"

namespace lampda {
namespace bsp {
namespace text_in {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

// only keep the chars inside a certain ascii range
bool is_ignore_char(char c) { return c < 32; }

Inputs read_inputs()
{
  Inputs ret;
  if (hal::serial::is_available() == 0)
  {
    uint8_t charRead = 0;

    // read available serial data
    do
    {
      // get the new byte:
      const char inChar = hal::serial::read();
      // if the incoming character is a newline, finish parsing
      if (inChar == '\n')
      {
        // do not add empty strings and null terminated only strings
        if (charRead != 0)
        {
          // add null termination if needed
          if (charRead < Inputs::maxCommandSize)
          {
            if (ret.commandList[ret.commandCount][charRead] != '\0')
              ret.commandList[ret.commandCount][charRead] = '\0';
          }
          else
          {
            ret.commandList[ret.commandCount][Inputs::maxCommandSize - 1] = '\0';
          }
          ret.commandCount += 1;
        }
        else
        {
          for (size_t i = 0; i < Inputs::maxCommandSize; i++)
            ret.commandList[ret.commandCount][i] = '\0';
        }

        charRead = 0;
      }
      else if (charRead < Inputs::maxCommandSize)
      {
        // add it to the inputString:
        if (not is_ignore_char(inChar))
        {
          ret.commandList[ret.commandCount][charRead] = inChar;
          charRead += 1;
        }
      }
    } while (hal::serial::is_available() == 0 && ret.commandCount < Inputs::maxCommands);
  }
  return ret;
}

} // namespace text_in
} // namespace bsp
} // namespace lampda
