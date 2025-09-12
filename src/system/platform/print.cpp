#ifndef PLATFORM_PRINT_CPP
#define PLATFORM_PRINT_CPP

#include "print.h"

#include <Arduino.h>

extern "C" {
  // hack to use prints in c files
#include "src/system/utils/print.h"
}

// mutex to prevent lockups
StaticSemaphore_t _PrintMutex;
SemaphoreHandle_t printMutex = xSemaphoreCreateMutexStatic(&_PrintMutex);

// only keep the chars inside a certain ascii range
bool is_ignore_char(char c) { return c < 32; }

void _lockPrintMutex(void) { xSemaphoreTake(printMutex, portMAX_DELAY); }
void _unlockPrintMutex(void) { xSemaphoreGive(printMutex); }

void init_prints() { Serial.begin(115200); }

void lampda_print(const char* format, ...)
{
  _lockPrintMutex();

  static char buffer[1024];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(buffer, format, argptr);
  va_end(argptr);

  Serial.print(millis());
  Serial.print("> ");
  Serial.println(buffer);

  _unlockPrintMutex();
}

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

std::vector<std::string> read_inputs()
{
  _lockPrintMutex();

  std::vector<std::string> ret;

  if (Serial.available())
  {
    uint8_t lineRead = 0;
    uint8_t charRead = 0;

    std::string inputString = "";

    // read available serial data
    do
    {
      // get the new byte:
      const char inChar = (char)Serial.read();
      // if the incoming character is a newline, finish parsing
      if (inChar == '\n')
      {
        // do not add empty strings and null terminated only strings
        if (inputString.size() != 0)
        {
          // add null termination if needed
          if (inputString[inputString.size() - 1] != '\0')
            inputString += '\0';

          ret.push_back(inputString);
        }

        inputString = "";
        lineRead += 1;
        charRead = 0;
      }
      else
      {
        // add it to the inputString:
        if (not is_ignore_char(inChar))
        {
          inputString += inChar;
        }
        charRead += 1;
      }
    } while (Serial.available() && lineRead < maxReadLinePerLoop && charRead < maxLineLenght);
  }

  _unlockPrintMutex();
  return ret;
}

#endif
