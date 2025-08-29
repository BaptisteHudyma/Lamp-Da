#include "src/system/platform/print.h"

#include <cstdio>
#include <iostream>

#include <string>
#include <vector>
#include <cstdarg>

#include "src/system/platform/time.h"
#include <mutex>

#define PLATFORM_PRINT_CPP

/**
 * \brief call once at program start
 */
void init_prints() {}

/**
 * \brief Print a screen to the external world
 */
std::mutex mut;
void lampda_print(const char* format, ...)
{
  std::scoped_lock(mut);

  static char buffer[1024];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(buffer, format, argptr);
  va_end(argptr);

  std::cout << time_ms() << "> " << buffer << std::endl;
}

/**
 * \breif read external inputs (may take some time)
 */
std::vector<std::string> read_inputs()
{
  std::vector<std::string> res;
  // TODO issue #132
  return res;
}
