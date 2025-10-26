#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../../platform/time.h"
#include "../../platform/threads.h"

#define WAKE_EVENT_SIGNAL  (1 << 2)
#define RESUME_TASK_SIGNAL (1 << 3)

  struct emu_task_t
  {
    volatile uint64_t timeout_us;
    volatile uint32_t event;
  };

  struct emu_task_t tasks;

  timestamp_t get_time(void)
  {
    timestamp_t t;
    t.val = time_ms() * 1000;
    t.val += time_us() % 1000;
    return t;
  }

  void task_scheduler(void)
  {
    // wait for eternity or until an event
    int result = wait_notification(0);
    // ignore all signals exept resume signals
    if (result & RESUME_TASK_SIGNAL)
    {
      // wait for the target event
      result = wait_notification(tasks.timeout_us / 1000);
      // after this, the task is timeout or resolved
      if (result & SCHED_NOTIFY_TIMER) // timeout
        tasks.event |= TASK_EVENT_TIMER;
      // resume the pd threads
      resume_thread(pd_taskName);
    }
  }

  uint32_t task_set_event(uint32_t event)
  {
    tasks.event |= event;
    notify_thread(taskScheduler_taskName, WAKE_EVENT_SIGNAL);
    return 0;
  }

  uint32_t task_wait_event(int timeout_us)
  {
    if (timeout_us > 0)
    {
      tasks.timeout_us = timeout_us;
    }
    else
    {
      // set max delay
      tasks.timeout_us = 0;
    }
    // resume scheduler
    notify_thread(taskScheduler_taskName, RESUME_TASK_SIGNAL);
    // then wait until event is set (or timeout)
    suspend_this_thread();

    /* Resume */
    uint32_t ret = tasks.event;
    tasks.event = 0;
    return ret;
  }

  uint32_t task_wait_event_mask(uint32_t event_mask, int timeout_us)
  {
    uint64_t deadline = get_time().val + timeout_us;
    uint32_t events = 0;
    uint64_t time_remaining_us = timeout_us;

    /* Add the timer event to the mask so we can indicate a timeout */
    event_mask |= TASK_EVENT_TIMER;

    while (!(events & event_mask))
    {
      /* Collect events to re-post later */
      events |= task_wait_event(time_remaining_us);

      time_remaining_us = deadline - get_time().val;
      if (timeout_us > 0 && time_remaining_us <= 0)
      {
        /* Ensure we return a TIMER event if we timeout */
        events |= TASK_EVENT_TIMER;
        break;
      }
    }

    /* Re-post any other events collected */
    if (events & ~event_mask)
      tasks.event |= (events & ~event_mask);

    return events & event_mask;
  }

  void task_clear_event_bitmap(uint32_t eventToClear) { tasks.event &= (~eventToClear); }

#ifdef __cplusplus
}
#endif
