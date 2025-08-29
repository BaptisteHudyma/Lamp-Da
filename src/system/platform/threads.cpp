#ifndef PLATFORM_THREADS_CPP
#define PLATFORM_THREADS_CPP

#include "threads.h"

#include "src/system/platform/print.h"

#include <Arduino.h>
#include <map>

// store all handles
std::map<const char* const, TaskHandle_t> handles;

// loop task
static void _redirect_task(void* arg)
{
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;

  while (1)
  {
    taskfunc();
    yield();
  }
}

// loop task
static void _redirect_suspend_task(void* arg)
{
  vTaskSuspend(NULL);
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;
  while (1)
  {
    taskfunc();
    yield();
  }
}

void start_thread(taskfunc_t taskFunction, const char* const taskName, const int priority, const int stackSize)
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
  if (pdPASS ==
      xTaskCreate(
              _redirect_task, taskName, max(configMINIMAL_STACK_SIZE, stackSize), (void*)taskFunction, prio, &handle))
  {
    handles[taskName] = handle;
  }
  else
  {
    lampda_print("task creation failed");
  }
}

void start_suspended_thread(taskfunc_t taskFunction,
                            const char* const taskName,
                            const int priority,
                            const int stackSize)
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
                            taskName,
                            max(configMINIMAL_STACK_SIZE, stackSize),
                            (void*)taskFunction,
                            prio,
                            &handle))
  {
    handles[taskName] = handle;
  }
  else
  {
    lampda_print("task %s creation failed", taskName);
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

void resume_thread(const char* const taskName)
{
  // handle already exists
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    lampda_print("ERROR: task handle %s do not exist", taskName);
    return;
  }

  vTaskResume(handle->second);
}

void notify_thread(const char* const taskName, int wakeUpEvent)
{
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    lampda_print("ERROR: task handle %s do not exist", taskName);
    return;
  }
  BaseType_t signal = pdFALSE;
  xTaskNotifyFromISR(handle->second, wakeUpEvent, eSetBits, &signal);
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

#endif
