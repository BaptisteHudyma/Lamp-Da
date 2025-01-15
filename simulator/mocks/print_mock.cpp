#include "src/system/platform/print.h"

#include <cstdio>
#include <iostream>

#include <string>
#include <vector>
#include <cstdarg>

#define PLATFORM_PRINT_CPP

/**
 * \brief call once at program start
 */
void init_prints() {}

/**
 * \brief Print a screen to the external world
 * To use with caution, this process can be slow
 */
void lampda_print(const std::string& str) { std::cout << str << std::endl; }
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
          std::cout << res << std::endl;
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
    std::cout << res << std::endl;
  }
  va_end(args);
}

/**
 * \breif read external inputs (may take some time)
 */
std::vector<std::string> read_inputs()
{
  std::vector<std::string> res;
  // TODO
  return res;
}
