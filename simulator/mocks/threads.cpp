#define PLATFORM_THREADS_CPP

#include "src/system/platform/threads.h"

#include "simulator/include/hardware_influencer.h"
#include <vector>

typedef void (*taskfunc_t)(void);
std::vector<taskfunc_t> threadPool;

namespace mock_registers {

void single_run_thread()
{
  for (auto& fun: threadPool)
  {
    fun();
  }
}

void run_threads()
{
// TODO issue #132: missing mocks for charger & pd negociator
#if 0
  // run until deep sleep
  while (not shouldStopThreads)
    single_run_thread();
#endif
}

} // namespace mock_registers

void start_thread(taskfunc_t taskFunction, const char* const taskName, const int priority, const int stackSize)
{
  threadPool.emplace_back(taskFunction);
}
void start_suspended_thread(taskfunc_t taskFunction,
                            const char* const taskName,
                            const int priority,
                            const int stackSize)
{
  threadPool.emplace_back(taskFunction);
}

void yield_this_thread() { mock_registers::single_run_thread(); }
void suspend_this_thread()
{
  // TODO issue #132
}

void suspend_all_threads()
{
  // TODO issue #132
}

void resume_thread(const char* const taskName) {}

void get_thread_debug(char* textBuff) {}
