#include "src/system/platform/threads.h"

#define PLATFORM_THREADS_CPP

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
{ // TODO
}

void resume_thread(const char* const taskName) {}
