#include "print.h"

#include <cstdarg>
#include <string>

void lampda_print(const char* fmt, ...)
{
  std::va_list args;
  va_start(args, fmt);

  std::string res = "";
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
          lampda_print(res);
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
    lampda_print(res);
  }
  va_end(args);
}