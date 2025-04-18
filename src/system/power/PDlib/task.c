#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../../platform/time.h"
#include "../../platform/threads.h"

  struct emu_task_t
  {
    int isWaiting;
    uint32_t event;
    timestamp_t wake_time;
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
    timestamp_t now = get_time();

    // task event is set, or timeout
    if (tasks.isWaiting && (tasks.event || now.val >= tasks.wake_time.val))
    {
      now = get_time();
      if (now.val >= tasks.wake_time.val)
      {
        tasks.event |= TASK_EVENT_TIMER;
      }
      tasks.wake_time.val = ~0ull;
      tasks.isWaiting = 0;
      // resume the pd threads
      resume_thread(pd_taskName);
      // suspend scheduler
      suspend_this_thread();
    }
    else
    {
      delay_ms(1);
    }
  }

  uint32_t task_set_event(uint32_t event)
  {
    tasks.event |= event;
    resume_thread(taskScheduler_taskName);
    return 0;
  }

  uint32_t task_wait_event(int timeout_us)
  {
    if (timeout_us > 0)
      tasks.wake_time.val = get_time().val + timeout_us;
    else
      // set max delay
      tasks.wake_time.val = (~0x00);

    tasks.isWaiting = 1;
    // resume scheduler
    resume_thread(taskScheduler_taskName);
    // then wait until event is set (or timeout)
    suspend_this_thread();

    /* Resume */
    int ret = tasks.event;
    tasks.event = 0;
    return ret;
  }

  uint32_t task_wait_event_mask(uint32_t event_mask, int timeout_us)
  {
    uint64_t deadline = get_time().val + timeout_us;
    uint32_t events = 0;
    int time_remaining_us = timeout_us;

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

  uint32_t* task_get_event_bitmap() { return &tasks.event; }

#ifdef __cplusplus
}
#endif