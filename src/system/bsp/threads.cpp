#ifndef HAL_THREADS_CPP
#define HAL_THREADS_CPP

#include "threads.h"

#include "src/system/hal/threads.h"

#include "src/system/bsp/text_out.h"

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

struct TaskHandleStorage
{
  TaskHandle_t taskHandle;
  uint32_t taskSize;
};

// store all handles
std::map<uint32_t, TaskHandleStorage> handles;

void low_level_start_thread(taskfunc_t taskFunction,
                            const uint32_t taskName,
                            const int priority,
                            const int stackSize,
                            const int startedSuspended)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
  {
    bsp::lampda_print("task %s (%d) creation failed: already exists", get_name_from_hash(taskName), taskName);
    return;
  }
  if (stackSize < 255)
  {
    bsp::lampda_print("task %s (%d) creation failed: stack too small", get_name_from_hash(taskName), taskName);
    return;
  }

  TaskHandle_t handle;
  const uint32_t createdTaskSize_bytes =
          hal::threads::HAL_create_thread(&handle,
                                          static_cast<hal::threads::taskfunc_t>(taskFunction),
                                          get_name_from_hash(taskName),
                                          priority,
                                          stackSize,
                                          startedSuspended);
  if (createdTaskSize_bytes > 0)
  {
    handles[taskName] = {handle, createdTaskSize_bytes};
  }
  else
  {
    bsp::lampda_print("task %s (%d) creation failed", get_name_from_hash(taskName), taskName);
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

void resume_thread(const uint32_t taskName)
{
  // handle already exists
  auto handle = __private::handles.find(taskName);
  if (handle == __private::handles.cend())
  {
    bsp::lampda_print("ERROR: resume task handle \'%s\' (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }

  hal::threads::HAL_resume_thread(handle->second.taskHandle);
}

uint16_t get_usage_percent(const uint32_t taskName)
{
  // handle already exists
  auto handle = __private::handles.find(taskName);
  if (handle == __private::handles.cend())
  {
    bsp::lampda_print(
            "ERROR: get usage percent handle \'%s\' (%d) do not exist", get_name_from_hash(taskName), taskName);
    return 0;
  }

  const int32_t taskStackSize = handle->second.taskSize;
  const int32_t taskHighWaterMark = hal::threads::HAL_get_task_high_water_mark_byte(handle->second.taskHandle);
  return static_cast<int16_t>(((taskStackSize - taskHighWaterMark) * 100) / taskStackSize);
}

void notify_thread(const uint32_t taskName, int wakeUpEvent)
{
  auto handle = __private::handles.find(taskName);
  if (handle == __private::handles.cend())
  {
    bsp::lampda_print("ERROR: notify task handle \'%s\' (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }
  hal::threads::HAL_notify_thread(handle->second.taskHandle, wakeUpEvent);
}

int wait_notification(const int timeout_ms) { return hal::threads::HAL_wait_notification(timeout_ms); }

void display_thread_debug()
{
  char buff[512];
  hal::threads::HAL_get_debug_thread_text(buff);
  bsp::lampda_print("%s", buff);

  bsp::lampda_print("---- Stack usage ----");
  for (const auto& handle: __private::handles)
  {
    const uint16_t usagePer = get_usage_percent(handle.first);
    bsp::lampda_print("%s: %d%%", get_name_from_hash(handle.first), usagePer);
  }
  bsp::lampda_print("----");
}

void shutdown()
{
  hal::threads::HAL_shutdown();
  __private::handles.clear();
}

} // namespace threads
} // namespace bsp
} // namespace lampda

#endif
