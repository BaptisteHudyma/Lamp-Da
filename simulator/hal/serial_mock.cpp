/*! \file print_mock.cpp
    \brief Mock of the board print and debugs
*/

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "simulator/include/hardware_influencer.h"

#include "src/system/utils/utils.h"

#include <cstdio>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#define HAL_SERIAL_CPP

namespace simulator {

std::vector<std::string> inputCommands;

class AsyncGetline
{
public:
  // AsyncGetline is a class that allows for asynchronous CLI getline-style input
  //(with 0% CPU usage!), which normal iostream usage does not easily allow.
  void start()
  {
    input = "";
    sendOverNextLine = true;
    continueGettingInput = true;

    // Start a new detached thread to call getline over and over again and retrieve new input to be processed.
    std::thread([&]() {
      // Non-synchronized string of input for the getline calls.
      std::string synchronousInput;
      char nextCharacter;

      // Get the asynchronous input lines.
      do
      {
        continueGettingInput = not simulator::mock_registers::shouldStopThreads;

        // Start with an empty line.
        synchronousInput = "";

        // Process input characters one at a time asynchronously, until a new line character is reached.
        while (continueGettingInput)
        {
          // See if there are any input characters available (asynchronously).
          while (std::cin.peek() == EOF)
          {
            // Ensure that the other thread is always yielded to when necessary. Don't sleep here;
            // only yield, in order to ensure that processing will be as responsive as possible.
            std::this_thread::yield();
          }

          // Get the next character that is known to be available.
          nextCharacter = std::cin.get();

          // Check for new line character.
          if (nextCharacter == '\n')
          {
            break;
          }

          // Since this character is not a new line character, add it to the synchronousInput string.
          synchronousInput += nextCharacter;
        }

        // Be ready to stop retrieving input at any moment.
        if (!continueGettingInput)
        {
          break;
        }

        // Wait until the processing thread is ready to process the next line.
        while (continueGettingInput && !sendOverNextLine)
        {
          // Ensure that the other thread is always yielded to when necessary. Don't sleep here;
          // only yield, in order to ensure that the processing will be as responsive as possible.
          std::this_thread::yield();
        }

        // Be ready to stop retrieving input at any moment.
        if (!continueGettingInput)
        {
          break;
        }

        // Safely send the next line of input over for usage in the processing thread.
        inputLock.lock();
        input = synchronousInput;
        inputLock.unlock();

        // Signal that although this thread will read in the next line,
        // it will not send it over until the processing thread is ready.
        sendOverNextLine = false;
      } while (continueGettingInput);
    }).detach();
  }

  // Stop getting asynchronous CLI input.
  ~AsyncGetline()
  {
    // Stop the getline thread.
    continueGettingInput = false;
  }

  // Get the next line of input if there is any; if not, sleep for a millisecond and return an empty string.
  std::string GetLine()
  {
    // See if the next line of input, if any, is ready to be processed.
    if (sendOverNextLine)
    {
      // Don't consume the CPU while waiting for input; this_thread::yield()
      // would still consume a lot of CPU, so sleep must be used.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      return "";
    }
    else
    {
      // Retrieve the next line of input from the getline thread and store it for return.
      inputLock.lock();
      std::string returnInput = input;
      inputLock.unlock();

      // Also, signal to the getline thread that it can continue
      // sending over the next line of input, if available.
      sendOverNextLine = true;

      return returnInput;
    }
  }

private:
  // Cross-thread-safe boolean to tell the getline thread to stop when AsyncGetline is deconstructed.
  std::atomic<bool> continueGettingInput;

  // Cross-thread-safe boolean to denote when the processing thread is ready for the next input line.
  // This exists to prevent any previous line(s) from being overwritten by new input lines without
  // using a queue by only processing further getline input when the processing thread is ready.
  std::atomic<bool> sendOverNextLine;

  // Mutex lock to ensure only one thread (processing vs. getline) is accessing the input string at a time.
  std::mutex inputLock;

  // string utilized safely by each thread due to the inputLock mutex.
  std::string input;
};

AsyncGetline get_line_async;
std::mutex mut;

void print_mock_loop()
{
  const std::string& s = get_line_async.GetLine();
  if (not s.empty())
  {
    std::scoped_lock lock(simulator::mut);
    inputCommands.push_back(s);
  }
}

size_t strIndex = 0;

} // namespace simulator

namespace lampda {
namespace hal {
namespace serial {

void init()
{
  simulator::get_line_async.start();
  bsp::threads::start_thread(simulator::print_mock_loop, utils::hash("tin_l"), 0, 255);
}

bool is_activated() { return true; }

bool is_available() { return simulator::inputCommands.empty() ? false : true; }

char read()
{
  if (not is_available())
  {
    bsp::lampda_print("Cannot read an empty char");
    return '\n';
  }

  const auto& str = simulator::inputCommands[0];
  if (simulator::strIndex < str.size())
  {
    return str[simulator::strIndex++];
  }
  else
  {
    simulator::strIndex = 0;
    simulator::inputCommands.erase(simulator::inputCommands.begin());
    return '\n';
  }
}

size_t write(const char* const buffer, size_t bufferSize)
{
  std::scoped_lock lock(simulator::mut);

  std::cout << std::string(buffer, bufferSize);
  return bufferSize;
}

} // namespace serial
} // namespace hal
} // namespace lampda
