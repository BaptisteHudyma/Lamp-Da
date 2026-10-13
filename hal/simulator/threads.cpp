/*! \file threads.cpp
    \brief Thread and tasks for the simulator
*/

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <iostream>
#include <mutex>

#include "src/system/hal/threads.h"

#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"

namespace simulator {

/// Latched notification state, one per thread
struct NotifyState
{
  std::mutex mutex;
  std::condition_variable cv;
  bool pending = false;
  int event = 0;
};

struct ThreadHandle
{
  std::thread fun;
  bool isSuspended;
  // shared_ptr keeps ThreadHandle movable (mutex and condition_variable are not)
  std::shared_ptr<NotifyState> notify = std::make_shared<NotifyState>();
};

std::map<size_t, ThreadHandle> threadPool;

} // namespace simulator

namespace lampda {
namespace hal {
namespace threads {

std::hash<std::thread::id> threadHasher;

uint32_t HAL_create_thread(TaskHandle_t* const handle,
                           taskfunc_t task,
                           char const* const name,
                           const int priority,
                           const int stackSize,
                           uint32_t* buffer,
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
  return stackSize;
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
  const auto it = simulator::threadPool.find(threadHasher(std::this_thread::get_id()));
  if (it == simulator::threadPool.cend())
  {
    // not a simulated task (e.g. main thread): fall back to a plain delay
    hal::delay_ms(timeout_ms <= 0 ? UINT16_MAX : timeout_ms);
    return SCHED_NOTIFY_TIMER;
  }

  // keep our own reference to the state
  const std::shared_ptr<simulator::NotifyState> state = it->second.notify;

  using clock = std::chrono::steady_clock;
  // wake up regularly to check the shutdown flag, so HAL_shutdown() can never hang
  static constexpr auto pollSlice = std::chrono::milliseconds(20);

  const bool waitForever = timeout_ms <= 0;
  const auto deadline = clock::now() + std::chrono::milliseconds(waitForever ? 0 : timeout_ms);

  std::unique_lock<std::mutex> lock(state->mutex);
  while (not state->pending)
  {
    if (simulator::mock_registers::shouldStopThreads)
      return SCHED_NOTIFY_TIMER;

    auto wakeAt = clock::now() + pollSlice;
    if (not waitForever)
    {
      if (clock::now() >= deadline)
        return SCHED_NOTIFY_TIMER;
      wakeAt = std::min(wakeAt, deadline);
    }
    state->cv.wait_until(lock, wakeAt);
  }

  // consume the notification
  state->pending = false;
  return state->event;
}

// Actions on  target threads

void HAL_notify_thread(TaskHandle_t handle, int wakeUpEvent)
{
  const auto it = simulator::threadPool.find((size_t)handle);
  if (it == simulator::threadPool.cend())
  {
    bsp::lampda_print("HAL_notify_thread> failed");
    return;
  }

  const std::shared_ptr<simulator::NotifyState> state = it->second.notify;
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    state->pending = true;
    state->event = wakeUpEvent;
  }
  state->cv.notify_one();
}

void HAL_suspend_thread(TaskHandle_t handle)
{
  auto h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    h->second.isSuspended = true;
  else
    bsp::lampda_print("HAL_suspend_thread> failed");
}

int HAL_is_suspended(TaskHandle_t handle)
{
  const auto& h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    return h->second.isSuspended ? 0 : 1;
  else
    bsp::lampda_print("HAL_is_suspended> failed");
  return 0;
}

void HAL_resume_thread(TaskHandle_t handle)
{
  auto h = simulator::threadPool.find((size_t)handle);
  if (h != simulator::threadPool.cend())
    h->second.isSuspended = false;
  else
    bsp::lampda_print("HAL_resume_thread> failed");
}

/// Return a task stack high water mark in bytes
uint32_t HAL_get_task_high_water_mark_byte(TaskHandle_t handle) { return 200; }

void HAL_get_debug_thread_text(char* textBuff) {}

void HAL_shutdown()
{
  std::cerr << "Initiating thread shutdown process..." << std::endl;

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
  std::cerr << "thread shutdown complete" << std::endl;
}

} // namespace threads
} // namespace hal
} // namespace lampda
