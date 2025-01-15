#ifndef PLATFORM_PRINT_CPP
#define PLATFORM_PRINT_CPP

#include "print.h"

#include <Arduino.h>

void init_prints() { Serial.begin(115200); }

void lampda_print(const std::string& str)
{
  Serial.print(millis());
  Serial.print("> ");
  Serial.println(str.c_str());
}

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

std::vector<std::string> read_inputs()
{
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
        ret.push_back(inputString);

        inputString = "";
        lineRead += 1;
        charRead = 0;
      }
      else
      {
        // add it to the inputString:
        inputString += inChar;
        charRead += 1;
      }
    } while (Serial.available() && lineRead < maxReadLinePerLoop && charRead < maxLineLenght);
  }
  return ret;
}

#endif