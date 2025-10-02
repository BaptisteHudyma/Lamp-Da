#include "src/system/platform/print.h"

#include <cstdio>
#include <iostream>

#include <string>
#include <vector>
#include <cstdarg>

#include "src/system/platform/time.h"
#include <mutex>
#include <thread>
#include <atomic>

#define PLATFORM_PRINT_CPP

extern "C" {
  // hack to use prints in c files
#include "src/system/utils/print.h"
}

std::vector<std::string> inputCommands;

std::atomic<bool> canRunInputThread = false; // TODO kill
std::thread inputThread;

/**
 * \brief call once at program start
 */
void init_prints()
{
  // spawn cin read thread
  canRunInputThread = true;
  inputThread = std::thread([&]() {
    while (canRunInputThread)
    {
      std::string s;
      // blocking call
      std::getline(std::cin, s, '\n');

      if (not s.empty())
      {
        std::scoped_lock(mut);
        inputCommands.push_back(s);
      }
    }
  });
}

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
 * \brief read external inputs (may take some time)
 */
std::vector<std::string> read_inputs()
{
  std::vector<std::string> res = inputCommands;
  inputCommands.clear();
  return res;
}
