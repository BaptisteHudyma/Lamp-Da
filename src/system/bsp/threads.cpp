#ifndef HAL_THREADS_CPP
#define HAL_THREADS_CPP

#include "threads.h"

#include "src/system/hal/print.h"
#include "src/system/hal/threads.h"

#include "src/system/utils/utils.h"

#include <map>

namespace lampda {
namespace bsp {
namespace threads {

// extern defines
const uint32_t pd_taskName = utils::hash("usbpd");
const uint32_t pdInterruptHandle_taskName = utils::hash("intpd");
const uint32_t button_taskName = utils::hash("button");
const uint32_t power_taskName = utils::hash("power");
const uint32_t user_taskName = utils::hash("user");
const uint32_t taskScheduler_taskName = utils::hash("task_sched");
const uint32_t sunset_taskName = utils::hash("sunset");
const uint32_t ble_cli_taskName = utils::hash("ble_cli");
const uint32_t print_taskName = utils::hash("print");

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
    case ble_cli_taskName:
      return "ble_cli";
    case print_taskName:
      return "print";
    default:
      break;
  }
  return "unknown";
}

namespace __private {

// store all handles
std::map<uint32_t, TaskHandle_t> handles;

void low_level_start_thread(taskfunc_t taskFunction,
                            const uint32_t taskName,
                            const int priority,
                            const int stackSize,
                            const int startedSuspended)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
  {
    hal::lampda_print("task %s (%d) creation failed: already exists", get_name_from_hash(taskName), taskName);
    return;
  }
  if (stackSize < 255)
  {
    hal::lampda_print("task %s (%d) creation failed: stack too small", get_name_from_hash(taskName), taskName);
    return;
  }

  TaskHandle_t handle;
  if (hal::threads::HAL_create_thread(&handle,
                                      static_cast<hal::threads::taskfunc_t>(taskFunction),
                                      get_name_from_hash(taskName),
                                      priority,
                                      stackSize,
                                      startedSuspended) == 0)
  {
    handles[taskName] = handle;
  }
  else
  {
    hal::lampda_print("task %s (%d) creation failed", get_name_from_hash(taskName), taskName);
  }
}

} // namespace __private

void start_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize)
{
  __private::low_level_start_thread(taskFunction, taskName, priority, stackSize, 1);
}

void start_suspended_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize)
{
  __private::low_level_start_thread(taskFunction, taskName, priority, stackSize, 0);
}

void yield_this_thread() { hal::threads::HAL_yield(); }

void suspend_this_thread() { hal::threads::HAL_suspend(); }

void suspend_all_threads()
{
  for (auto handle: __private::handles)
  {
    hal::threads::HAL_suspend_thread(handle.second);
  }
}

int is_all_suspended()
{
  for (const auto& handle_it: __private::handles)
  {
    if (hal::threads::HAL_is_suspended(handle_it.second) != 0)
    {
      return 1;
    }
  }
  return 0;
}

void resume_thread(const uint32_t taskName)
{
  // handle already exists
  auto handle = __private::handles.find(taskName);
  if (handle == __private::handles.cend())
  {
    hal::lampda_print("ERROR: resume task handle \'%s\' (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }

  hal::threads::HAL_resume_thread(handle->second);
}

void notify_thread(const uint32_t taskName, int wakeUpEvent)
{
  auto handle = __private::handles.find(taskName);
  if (handle == __private::handles.cend())
  {
    hal::lampda_print("ERROR: notify task handle \'%s\' (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }
  hal::threads::HAL_notify_thread(handle->second, wakeUpEvent);
}

int wait_notification(const int timeout_ms) { return hal::threads::HAL_wait_notification(timeout_ms); }

void get_thread_debug(char* textBuff) { hal::threads::HAL_get_debug_thread_text(textBuff); }

void shutdown()
{
  hal::threads::HAL_shutdown();
  __private::handles.clear();
}

} // namespace threads
} // namespace bsp
} // namespace lampda

#endif
