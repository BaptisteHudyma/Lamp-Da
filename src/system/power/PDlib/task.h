#ifndef PD_TASK_H
#define PD_TASK_H

#include <stdint.h>

#define TASK_EVENT_PD_AWAKE (1 << 18)

/* task_wake() called on task */
#define TASK_EVENT_WAKE (1 << 29)

/*
 * Timer expired.  For example, task_wait_event() timed out before receiving
 * another event.
 */
#define TASK_EVENT_TIMER (1U << 31)

typedef union
{
  uint64_t val;
  struct
  {
    uint32_t lo;
    uint32_t hi;
  } le /* little endian words */;
} timestamp_t;

// Get the current timestamp from the system timer.
timestamp_t get_time(void);

// call in a dedicated task
void task_scheduler(void);

/**
 * Set a task event.
 *
 * If the task is higher priority than the current task, this will cause an
 * immediate context switch to the new task.
 *
 * Can be called both in interrupt context and task context.
 *
 * @param event		Event bitmap to set (TASK_EVENT_*)
 * @return		The bitmap of events which occurred if wait!=0, else 0.
 */
uint32_t task_set_event(uint32_t event);

/**
 * Wait for the next event.
 *
 * If one or more events are already pending, returns immediately.  Otherwise,
 * it de-schedules the calling task and wakes up the next one in the priority
 * order.  Automatically clears the bitmap of received events before returning
 * the events which are set.
 *
 * @param timeout_us	If > 0, sets a timer to produce the TASK_EVENT_TIMER
 *			event after the specified micro-second duration.
 *
 * @return The bitmap of received events.
 */
uint32_t task_wait_event(int timeout_us);

/**
 * Wait for any event included in an event mask.
 *
 * If one or more events are already pending, returns immediately.  Otherwise,
 * it de-schedules the calling task and wakes up the next one in the priority
 * order.  Automatically clears the bitmap of received events before returning
 * the events which are set.
 *
 * @param event_mask	Bitmap of task events to wait for.
 *
 * @param timeout_us	If > 0, sets a timer to produce the TASK_EVENT_TIMER
 *			event after the specified micro-second duration.
 *
 * @return		The bitmap of received events. Includes
 *			TASK_EVENT_TIMER if the timeout is reached.
 */
uint32_t task_wait_event_mask(uint32_t event_mask, int timeout_us);

/**
 * Wake a task.  This sends it the TASK_EVENT_WAKE event.
 */
static inline void task_wake() { task_set_event(TASK_EVENT_WAKE); }

/**
 * Return a pointer to the bitmap of events of the task.
 */
uint32_t* task_get_event_bitmap();

#endif