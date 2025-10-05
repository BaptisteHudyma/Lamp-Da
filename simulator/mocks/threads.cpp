#define PLATFORM_THREADS_CPP

#include "src/system/platform/threads.h"
#include "src/system/platform/time.h"

#include "simulator/include/hardware_influencer.h"
#include <vector>
#include <thread>

typedef void (*taskfunc_t)(void);
std::vector<std::thread> threadPool;

bool isSuspended = false;

void start_thread(taskfunc_t taskFunction, const char* const taskName, const int priority, const int stackSize)
{
  // ALWAYS CAPTURE taskFunction EXPLICITLY
  threadPool.emplace_back(std::thread([taskFunction]() {
    while (taskFunction and not mock_registers::shouldStopThreads)
    {
      if (not isSuspended)
        taskFunction();
      delay_ms(1);
    }
  }));
}
void start_suspended_thread(taskfunc_t taskFunction,
                            const char* const taskName,
                            const int priority,
                            const int stackSize)
{
  // ALWAYS CAPTURE taskFunction EXPLICITLY
  threadPool.emplace_back(std::thread([taskFunction]() {
    while (taskFunction and not mock_registers::shouldStopThreads)
    {
      if (not isSuspended)
        taskFunction();
      delay_ms(1);
    }
  }));
}

void yield_this_thread()
{
  // TODO issue #132
}

void suspend_this_thread()
{
  // TODO issue #132
}

void suspend_all_threads() { isSuspended = true; }

int is_all_suspended() { return isSuspended ? 1 : 0; }

void resume_thread(const char* const taskName) {}

void notify_thread(const char* const taskName, int wakeUpEvent) {};
int wait_notification(const int timeout_ms) { return 0; }

void get_thread_debug(char* textBuff) {}
