#ifndef MOCK_PRINT_H
#define MOCK_PRINT_H

#include <cstdio>
#include <iostream>
#define PLATFORM_PRINT_CPP

#include <string>
#include <vector>
#include <cstdarg>

#include "src/system/platform/print.h"

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
  va_list args;
  va_start(args, fmt);
  printf(fmt, args);
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

#endif