#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../../platform/time.h"

  struct emu_task_t
  {
    uint8_t resume;
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

    if (tasks.event || now.val >= tasks.wake_time.val)
    {
      now = get_time();
      if (now.val >= tasks.wake_time.val)
        tasks.event |= TASK_EVENT_TIMER;
      tasks.wake_time.val = ~0ull;
      tasks.resume = 1;
    }
    delay_ms(1);
  }

  uint32_t task_set_event(uint32_t event)
  {
    tasks.event |= event;
    return 0;
  }

  uint32_t task_wait_event(int timeout_us)
  {
    if (timeout_us > 0)
      tasks.wake_time.val = get_time().val + timeout_us;

    tasks.resume = 0;
    while (!tasks.resume)
    {
      delay_ms(1);
    }

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

#ifdef __cplusplus
}
#endif