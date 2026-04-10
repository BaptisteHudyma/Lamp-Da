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

void lampda_print_raw(const char* format, ...)
{
  std::scoped_lock(mut);

  static char buffer[1024];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(buffer, format, argptr);
  va_end(argptr);

  std::cout << buffer;
}

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
Inputs read_inputs()
{
  Inputs res;
  res.commandCount = 0;

  for (const auto& command: inputCommands)
  {
    if (res.commandCount >= Inputs::maxCommands)
    {
      std::cerr << "Command count exceeded max commands, ignoring following commands" << std::endl;
      break;
    }
    if (command.size() >= Inputs::maxCommandSize)
    {
      std::cerr << "Command exceeded max command lenght" << std::endl;
      continue;
    }

    size_t cnt = 0;
    for (const char& c: command)
    {
      res.commandList[res.commandCount][cnt] = c;
      cnt += 1;
    }
    res.commandList[res.commandCount][cnt] = '\0';
    res.commandCount += 1;
  }

  // clear
  inputCommands.clear();
  return res;
}
