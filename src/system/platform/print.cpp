#ifndef PLATFORM_PRINT_CPP
#define PLATFORM_PRINT_CPP

#include "print.h"

#include <Arduino.h>
#include <cstdarg>

void init_prints() { Serial.begin(115200); }

void lampda_print(const std::string& str) { Serial.println(str.c_str()); }
void lampda_print(const char* fmt, ...)
{
  std::va_list args;
  va_start(args, fmt);

  std::string res;
  for (const char* p = fmt; *p != '\0'; ++p)
  {
    switch (*p)
    {
      case '%':
        switch (*++p) // read format symbol
        {
          case 'i':
          case 'd':
            res += std::to_string(va_arg(args, int));
            continue;
          case 'f':
            res += std::to_string(va_arg(args, double));
            continue;
          case 's':
            res += va_arg(args, const char*);
            continue;
          case 'c':
            res += static_cast<char>(va_arg(args, int));
            continue;
          case '%':
            res += '%';
            continue;
            /* ...more cases... */
        }
      case '\n':
        {
          Serial.println(res.c_str());
          res = "";
          ++p;
          break;
        }
      default:
        break; // no formatting to do
    }
    res += (*p);
  }
  if (not res.empty())
  {
    Serial.println(res.c_str());
  }
  va_end(args);
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