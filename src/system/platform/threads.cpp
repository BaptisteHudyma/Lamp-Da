#ifndef PLATFORM_THREADS_CPP
#define PLATFORM_THREADS_CPP

#include "threads.h"

#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"

#include <Arduino.h>
#include <map>

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

// store all handles
std::map<uint32_t, TaskHandle_t> handles;

// loop task
[[noreturn]] static void _redirect_task(void* arg)
{
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;

  while (true)
  {
    taskfunc();
    yield();
  }
}

// loop task
[[noreturn]] static void _redirect_suspend_task(void* arg)
{
  vTaskSuspend(NULL);
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;
  while (true)
  {
    taskfunc();
    yield();
  }
}

void start_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
    return;

  uint32_t prio = TASK_PRIO_LOW;
  if (priority >= 2)
    prio = TASK_PRIO_HIGH;
  else if (priority >= 1)
    prio = TASK_PRIO_NORMAL;
  else
    prio = TASK_PRIO_LOW;

  TaskHandle_t handle;
  if (pdPASS == xTaskCreate(_redirect_task,
                            get_name_from_hash(taskName),
                            max<int>(configMINIMAL_STACK_SIZE, stackSize),
                            (void*)taskFunction,
                            prio,
                            &handle))
  {
    handles[taskName] = handle;
  }
  else
  {
    platform::lampda_print("task creation failed");
  }
}

void start_suspended_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize)
{
  // handle already exists
  if (handles.find(taskName) != handles.cend())
    return;

  uint32_t prio = TASK_PRIO_LOW;
  if (priority >= 2)
    prio = TASK_PRIO_HIGH;
  else if (priority >= 1)
    prio = TASK_PRIO_NORMAL;
  else
    prio = TASK_PRIO_LOW;

  TaskHandle_t handle;
  if (pdPASS == xTaskCreate(_redirect_suspend_task,
                            get_name_from_hash(taskName),
                            max<int>(configMINIMAL_STACK_SIZE, stackSize),
                            (void*)taskFunction,
                            prio,
                            &handle))
  {
    handles[taskName] = handle;
  }
  else
  {
    platform::lampda_print("task %s (%d) creation failed", get_name_from_hash(taskName), taskName);
  }
}

void yield_this_thread() { yield(); }

void suspend_this_thread() { vTaskSuspend(NULL); }

void suspend_all_threads()
{
  for (auto handle: handles)
  {
    // notify to cancel timeouts
    xTaskNotifyGive(handle.second);
    vTaskSuspend(handle.second);
  }
}

int is_all_suspended()
{
  for (const auto& handle_it: handles)
  {
    if (eTaskGetState(handle_it.second) != eTaskState::eSuspended)
    {
      return 0;
    }
  }
  return 1;
}

void resume_thread(const uint32_t taskName)
{
  // handle already exists
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    platform::lampda_print("ERROR: task handle %s (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }

  if (isInISR())
  {
    xTaskResumeFromISR(handle->second);
  }
  else
  {
    vTaskResume(handle->second);
  }
}

void notify_thread(const uint32_t taskName, int wakeUpEvent)
{
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    platform::lampda_print("ERROR: task handle %s (%d) do not exist", get_name_from_hash(taskName), taskName);
    return;
  }
  if (isInISR())
  {
    BaseType_t signal = pdFALSE;
    xTaskNotifyFromISR(handle->second, wakeUpEvent, eSetBits, &signal);
  }
  else
  {
    vTaskSuspendAll();
    xTaskNotify(handle->second, wakeUpEvent, eSetBits);
    (void)xTaskResumeAll();
  }
}

int wait_notification(const int timeout_ms)
{
  uint32_t notifiedValue = 0;
  BaseType_t result;

  if (timeout_ms <= 0)
    result = xTaskNotifyWait(0,              // don't clear on entry
                             UINT32_MAX,     // clear all bits on exit
                             &notifiedValue, // returned value
                             portMAX_DELAY);
  else
    result = xTaskNotifyWait(0,              // don't clear on entry
                             UINT32_MAX,     // clear all bits on exit
                             &notifiedValue, // returned value
                             ms2tick(timeout_ms));

  if (result == pdFALSE)
  {
    // timeout
    notifiedValue |= SCHED_NOTIFY_TIMER;
  }

  return notifiedValue;
}

void get_thread_debug(char* textBuff) { vTaskList(textBuff); }

} // namespace threads
} // namespace platform
} // namespace lampda

#endif
