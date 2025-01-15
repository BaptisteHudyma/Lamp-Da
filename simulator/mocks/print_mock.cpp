#include "src/system/platform/print.h"

#include <cstdio>
#include <iostream>

#include <string>
#include <vector>
#include <cstdarg>

#include "src/system/platform/time.h"

#define PLATFORM_PRINT_CPP

/**
 * \brief call once at program start
 */
void init_prints() {}

/**
 * \brief Print a screen to the external world
 */
void lampda_print(const std::string& str) { std::cout << time_ms() << "> " << str << std::endl; }

/**
 * \breif read external inputs (may take some time)
 */
std::vector<std::string> read_inputs()
{
  std::vector<std::string> res;
  // TODO
  return res;
}
