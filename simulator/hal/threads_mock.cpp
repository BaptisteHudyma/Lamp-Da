/*! \file threads_mock.cpp
    \brief Mock of the board threads and tasks
*/
#define HAL_THREADS_CPP

#include <functional>
#include <map>
#include <thread>

#include "src/system/hal/threads.h"

#include "src/system/hal/time.h"
#include "src/system/hal/print.h"

#include "src/system/utils/utils.h"

#include "src/system/bsp/threads.h"

#include "simulator/include/hardware_influencer.h"

namespace simulator {

struct ThreadHandle
{
  std::thread fun;
  bool isSuspended;
};

std::map<size_t, ThreadHandle> threadPool;

} // namespace simulator

namespace lampda {
namespace hal {
namespace threads {

std::hash<std::thread::id> threadHasher;

int HAL_create_thread(TaskHandle_t* const handle,
                      taskfunc_t task,
                      char const* const name,
                      const int priority,
                      const int stackSize,
                      const int startSuspended)
{
  // ALWAYS CAPTURE taskFunction EXPLICITLY
  simulator::ThreadHandle h;
  h.fun = std::thread([task]() {
    while (task and not simulator::mock_registers::shouldStopThreads)
    {
      // mock suspend: if flag is false, dont run the function
      const auto& h = simulator::threadPool.find(threadHasher(std::this_thread::get_id()));
      if (h != simulator::threadPool.cend() && not h->second.isSuspended)
        task();
      hal::delay_ms(1);
    }
  });
  h.isSuspended = startSuspended == 0;

  const size_t id = threadHasher(h.fun.get_id());
  simulator::threadPool[id] = std::move(h);

  // set handle
  *handle = (TaskHandle_t)id;
  return 0;
}

// Actions on this thread

void HAL_yield() { std::this_thread::yield(); }

void HAL_suspend()
{
  auto h = simulator::threadPool.find(threadHasher(std::this_thread::get_id()));
  if (h != simulator::threadPool.cend())
    h->second.isSuspended = true;
}

int HAL_wait_notification(const int timeout_ms)
{
  // TODO issue #132 support when implemented

  // Unsupported yet
  if (timeout_ms <= 0)
    hal::delay_ms(UINT16_MAX);
  else
    hal::delay_ms(timeout_ms);

  // systematical timeout
  return SCHED_NOTIFY_TIMER;
}

// Actions on  target threads

void HAL_notify_thread(TaskHandle_t handle, int wakeUpEvent) {
  // TODO issue #132 support when implemented
};

void HAL_suspend_thread(TaskHandle_t handle)
{
  auto h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    h->second.isSuspended = true;
  else
    hal::lampda_print("HAL_suspend_thread> failed");
}

int HAL_is_suspended(TaskHandle_t handle)
{
  const auto& h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    return h->second.isSuspended ? 0 : 1;
  else
    hal::lampda_print("HAL_is_suspended> failed");
  return 0;
}

void HAL_resume_thread(TaskHandle_t handle)
{
  auto h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    h->second.isSuspended = false;
  else
    hal::lampda_print("HAL_resume_thread> failed");
}
void HAL_get_debug_thread_text(char* textBuff) {}

void HAL_shutdown()
{
  hal::lampda_print("Initiating thread shutdown process...");

  simulator::mock_registers::shouldStopThreads = true;
  for (auto& [id, thread]: simulator::threadPool)
  {
    while (not thread.fun.joinable())
    {
      hal::delay_ms(1);
    }

    thread.fun.join();
  }
  simulator::threadPool.clear();
  hal::lampda_print("thread shutdown complete");
}

} // namespace threads
} // namespace hal
} // namespace lampda
