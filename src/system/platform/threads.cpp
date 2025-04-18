#ifndef PLATFORM_THREADS_CPP
#define PLATFORM_THREADS_CPP

#include "threads.h"
#include "print.h"

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
    lampda_print("task creation failed");
  }
}

void yield_this_thread() { yield(); }

void suspend_this_thread() { vTaskSuspend(NULL); }

void suspend_all_threads()
{
  for (auto handle: handles)
  {
    vTaskSuspend(handle.second);
  }
}

void resume_thread(const char* const taskName)
{
  // handle already exists
  auto handle = handles.find(taskName);
  if (handle == handles.cend())
  {
    lampda_print("task handle do not exist");
    return;
  }

  vTaskResume(handle->second);
}

void get_thread_debug(char* textBuff) { vTaskList(textBuff); }

#endif