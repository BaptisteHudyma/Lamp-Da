/*! \file threads.h
    \brief Interface for the platform specific tasks and threads.
*/

// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_THREADS_H
#define PLATFORM_THREADS_H

#ifdef __cplusplus

#include <cstdint>

namespace lampda {
namespace platform {
/// Define tasks and threads specifics.
namespace threads {

extern "C" {
#endif

#define SCHED_NOTIFY_TIMER (1 << 0) // reserved event mask

  // store tasks names here
  /// name of the USB power delivery task
  extern const uint32_t pd_taskName;
  /// name of the USB power delivery interrupt handle task
  extern const uint32_t pdInterruptHandle_taskName;
  /// name of the main power task
  extern const uint32_t power_taskName;
  /// name of the optional user task
  extern const uint32_t user_taskName;
  /// name of the task scheduling task
  extern const uint32_t taskScheduler_taskName;
  // name of the task t schedule sunset
  extern const uint32_t sunset_taskName;

  /// model of a task function
  typedef void (*taskfunc_t)(void);

  /**
   * \brief Start a separate thread, running until the system shuts off
   * \param taskFunction the function to run
   * \param[in] taskName The name associated
   * \param[in] priority from  to 2, this thread priority
   * \param[in] stackSize The size of the stack to allocate. can be ignored and checked while running using the command
   * line
   */
  extern void start_thread(taskfunc_t taskFunction, const uint32_t taskName, const int priority, const int stackSize);
  /**
   * \brief Start a separate thread, running until the system shuts off. Start in suspended state.
   * \param taskFunction the function to run
   * \param[in] taskName The name associated
   * \param[in] priority from  to 2, this thread priority
   * \param[in] stackSize The size of the stack to allocate. can be ignored and checked while running using the command
   * line
   */
  extern void start_suspended_thread(taskfunc_t taskFunction,
                                     const uint32_t taskName,
                                     const int priority,
                                     const int stackSize);

  /// make this thread pass the control to other threads
  extern void yield_this_thread();

  /// threads can only suspend itself
  extern void suspend_this_thread();

  /// Suspend all threads
  /// \warning: Can deadlock the system if called from a subthread.
  extern void suspend_all_threads();

  /**
   * \brief check that all threads are suspended (mandatory for sleep mode)
   * \return 1 for success, 0 for failure
   */
  extern int is_all_suspended();

  /**
   * \brief resume a target thread
   * \param[in] taskName target task name
   */
  extern void resume_thread(const uint32_t taskName);

  /**
   * \brief notify a thread to resume
   * \param[in] taskName target task name
   * \param[in] wakeUpEvent type of the event to send
   */
  extern void notify_thread(const uint32_t taskName, int wakeUpEvent);

  /**
   * \brief block this thread until a timeout or notification is received
   * \param[in] timeout_ms Timeout delay. Can be zero of less for no timeout
   * \return the wake up flag
   */
  extern int wait_notification(const int timeout_ms);

  // compute and return a debug for threads
  extern void get_thread_debug(char* textBuff);

#ifdef __cplusplus
}

} // namespace: threads
} // namespace: platform
} // namespace: lampda
#endif

#endif
