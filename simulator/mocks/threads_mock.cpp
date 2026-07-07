/*! \file threads_mock.cpp
    \brief Mock of the board threads and tasks
*/

#define PLATFORM_THREADS_CPP

#include <set>

#include "src/system/platform/threads.h"

#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"
#include <vector>
#include <thread>

namespace simulator {
std::vector<std::thread> threadPool;

bool isSuspended = false;
} // namespace simulator

namespace lampda {
namespace platform {
namespace threads {

// extern defines
const uint32_t pd_taskName = utils::hash("usbpd");
const uint32_t pdInterruptHandle_taskName = utils::hash("intpd");
const uint32_t button_taskName = utils::hash("button");
const uint32_t power_taskName = utils::hash("power");
const uint32_t user_taskName = utils::hash("user");
const uint32_t taskScheduler_taskName = utils::hash("task_sched");
const uint32_t sunset_taskName = utils::hash("sunset");

const char* const get_name_from_hash(const uint32_t hash)
{
  switch (hash)
  {
    case pd_taskName:
      return "usbpd";
    case pdInterruptHandle_taskName:
      return "intpd";
    case button_taskName:
      return "button";
    case power_taskName:
      return "power";
    case user_taskName:
      return "user";
    case taskScheduler_taskName:
      return "task_sched";
    case sunset_taskName:
      return "sunset";
    default:
      break;
  }
  return "unknown";
}

typedef void (*taskfunc_t)(void);

std::set<uint32_t> handles;

void start_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
    return;

  // ALWAYS CAPTURE taskFunction EXPLICITLY
  simulator::threadPool.emplace_back(std::thread([taskFunction]() {
    while (taskFunction and not simulator::mock_registers::shouldStopThreads)
    {
      if (not simulator::isSuspended)
        taskFunction();
      platform::delay_ms(1);
    }
  }));

  handles.emplace(taskName);
}
void start_suspended_thread(taskfunc_t taskFunction, uint32_t taskName, const int priority, const int stackSize)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
    return;

  // ALWAYS CAPTURE taskFunction EXPLICITLY
  simulator::threadPool.emplace_back(std::thread([taskFunction]() {
    while (taskFunction and not simulator::mock_registers::shouldStopThreads)
    {
      if (not simulator::isSuspended)
        taskFunction();
      platform::delay_ms(1);
    }
  }));

  handles.emplace(taskName);
}

void yield_this_thread()
{
  // TODO issue #132
}

void suspend_this_thread()
{
  // TODO issue #132
}

void suspend_all_threads() { simulator::isSuspended = true; }

int is_all_suspended() { return simulator::isSuspended ? 1 : 0; }

void resume_thread(const uint32_t taskName)
{
  // handle already exists
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    platform::lampda_print("ERROR: task handle %s (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }
}

void notify_thread(const uint32_t, int wakeUpEvent) {};
int wait_notification(const int timeout_ms) { return 0; }

void get_thread_debug(char* textBuff) {}

} // namespace threads
} // namespace platform
} // namespace lampda
