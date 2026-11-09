#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../../src/system/hal/time.h"
#include "../../../../src/system/bsp/threads.h"

#define WAKE_EVENT_SIGNAL (1 << 2)

  /**
   * Pending events of the PD task.
   * Set by any thread (interrupt handler, main loop), consumed by the PD task: always accessed atomically.
   *
   * The PD task sleeps on its own thread notification, that task_set_event sends. A notification is latched by the OS
   * when the task is not waiting yet, so a wake up can never be lost.
   * This replaces a suspend/resume scheme with a scheduler thread, where a resume sent before the PD task was
   * suspended was dropped, freezing the PD state machine forever (issues #357, #642)
   */
  static volatile uint32_t pendingEvents = 0;

  timestamp_t get_time(void)
  {
    timestamp_t t;
    t.val = (uint64_t)time_ms() * 1000;
    t.val += time_us() % 1000;
    return t;
  }

  uint32_t task_timeout_us_to_ms(uint64_t timeout_us)
  {
    if (timeout_us == 0)
      return 0;
    // round up: a truncated sub millisecond timeout would be 0, which waits forever (issue #642)
    return (uint32_t)((timeout_us + 999) / 1000);
  }

  uint32_t task_set_event(uint32_t event)
  {
    __atomic_fetch_or(&pendingEvents, event, __ATOMIC_SEQ_CST);
    notify_thread(pd_taskName, WAKE_EVENT_SIGNAL);
    return 0;
  }

  uint32_t task_wait_event(int timeout_us)
  {
    // events already pending: return immediately
    if (__atomic_load_n(&pendingEvents, __ATOMIC_SEQ_CST) == 0)
    {
      // 0 is an infinite wait
      const uint32_t timeout_ms = (timeout_us > 0) ? task_timeout_us_to_ms((uint64_t)timeout_us) : 0;
      // can return early, on a notification of an event already consumed: callers handle spurious wake ups
      if (wait_notification((int)timeout_ms) & SCHED_NOTIFY_TIMER)
        __atomic_fetch_or(&pendingEvents, TASK_EVENT_TIMER, __ATOMIC_SEQ_CST);
    }

    return __atomic_exchange_n(&pendingEvents, 0u, __ATOMIC_SEQ_CST);
  }

  uint32_t task_wait_event_mask(uint32_t event_mask, int timeout_us)
  {
    const uint64_t deadline = get_time().val + (uint64_t)((timeout_us > 0) ? timeout_us : 0);
    uint32_t events = 0;
    int time_remaining_us = timeout_us;

    /* Add the timer event to the mask so we can indicate a timeout */
    event_mask |= TASK_EVENT_TIMER;

    while (!(events & event_mask))
    {
      /* Collect events to re-post later */
      events |= task_wait_event(time_remaining_us);

      if (timeout_us > 0)
      {
        // signed: an unsigned remaining time underflowed once the deadline passed, and became an infinite wait
        const int64_t remaining = (int64_t)(deadline - get_time().val);
        if (remaining <= 0)
        {
          /* Ensure we return a TIMER event if we timeout */
          events |= TASK_EVENT_TIMER;
          break;
        }
        time_remaining_us = (int)remaining;
      }
    }

    /* Re-post any other events collected */
    if (events & ~event_mask)
      __atomic_fetch_or(&pendingEvents, events & ~event_mask, __ATOMIC_SEQ_CST);

    return events & event_mask;
  }

  void task_clear_event_bitmap(uint32_t eventToClear)
  {
    __atomic_fetch_and(&pendingEvents, ~eventToClear, __ATOMIC_SEQ_CST);
  }

#ifdef __cplusplus
}
#endif
