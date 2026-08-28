#ifndef HAL_THREADS_CPP
#define HAL_THREADS_CPP

#include "threads.h"

#include "src/system/bsp/threads.h"

#include <Arduino.h>

namespace lampda {
namespace hal {
namespace threads {

volatile bool canRun = true;

// loop task
[[noreturn]] static void _redirect_task(void* arg)
{
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;

  while (canRun)
  {
    taskfunc();
    yield();
  }
}

// loop task
[[noreturn]] static void _redirect_suspend_task(void* arg)
{
  SchedulerRTOS::taskfunc_t taskfunc = (SchedulerRTOS::taskfunc_t)arg;

  HAL_suspend();
  while (canRun)
  {
    taskfunc();
    yield();
  }
}

uint32_t HAL_create_thread(TaskHandle_t* const handle,
                           taskfunc_t task,
                           char const* const name,
                           const int priority,
                           const int stackSize,
                           const int startSuspended)
{
  uint32_t prio = TASK_PRIO_LOW;
  if (priority >= 2)
    prio = TASK_PRIO_HIGH;
  else if (priority >= 1)
    prio = TASK_PRIO_NORMAL;
  else
    prio = TASK_PRIO_LOW;

  auto& taskToUse = startSuspended == 0 ? _redirect_suspend_task : _redirect_task;

  const uint16_t taskSize = max<int>(configMINIMAL_STACK_SIZE, stackSize);
  if (pdPASS == xTaskCreate(taskToUse, name, taskSize, (void*)task, prio, handle))
  {
    return taskSize * sizeof(StackType_t);
  }
  return 0;
}

// Actions on this thread

void HAL_yield() { yield(); }

void HAL_suspend() { vTaskSuspend(NULL); }

int HAL_wait_notification(const int timeout_ms)
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

// Actions on  target threads

void HAL_notify_thread(TaskHandle_t handle, int wakeUpEvent)
{
  if (isInISR())
  {
    BaseType_t signal = pdFALSE;
    xTaskNotifyFromISR(handle, wakeUpEvent, eSetBits, &signal);
  }
  else
  {
    xTaskNotify(handle, wakeUpEvent, eSetBits);
  }
}

void HAL_suspend_thread(TaskHandle_t handle) { vTaskSuspend(handle); }

int HAL_is_suspended(TaskHandle_t handle)
{
  const auto state = eTaskGetState(handle);
  if (state == eTaskState::eSuspended)
    return 0;
  return 1;
}

void HAL_resume_thread(TaskHandle_t handle)
{
  if (isInISR())
  {
    xTaskResumeFromISR(handle);
  }
  else
  {
    vTaskResume(handle);
  }
}

uint32_t HAL_get_task_high_water_mark_byte(TaskHandle_t handle)
{
  UBaseType_t hwm = uxTaskGetStackHighWaterMark(handle);
  return hwm * sizeof(StackType_t);
}

void HAL_get_debug_thread_text(char* textBuff) { vTaskList(textBuff); }

void HAL_shutdown() { canRun = false; }

} // namespace threads
} // namespace hal
} // namespace lampda

#endif
