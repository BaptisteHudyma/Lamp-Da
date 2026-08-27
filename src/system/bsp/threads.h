/*! \file bsp/threads.h
 *  \brief Low level thread handling.
 */
#pragma once

#define SCHED_NOTIFY_TIMER (1 << 0) // reserved event mask

#ifdef __cplusplus

#include <cstdint>

namespace lampda {
namespace bsp {
/// Define low level thread behavior and handling
namespace threads {

extern "C" {
#endif

  typedef void* TaskHandle_t;

  // store tasks names here
  /// name of the USB power delivery task
  extern const uint32_t pd_taskName;
  /// name of the USB power delivery interrupt handle task
  extern const uint32_t pdInterruptHandle_taskName;
  // Name of the button handling task
  extern const uint32_t button_taskName;
  /// name of the main power task
  extern const uint32_t power_taskName;
  /// name of the optional user task
  extern const uint32_t user_taskName;
  /// name of the task scheduling task
  extern const uint32_t taskScheduler_taskName;
  /// name of the task schedule sunset
  extern const uint32_t sunset_taskName;
  /// BLE cli queue
  extern const uint32_t ble_cli_taskName;
  /// UART print task
  extern const uint32_t print_taskName;

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

  /**
   * \brief check that all threads are suspended (mandatory for sleep mode)
   * \return 0 for success, any other for failure
   */
  extern int is_all_suspended();

  /**
   * \brief resume a target thread
   * \param[in] taskName target task name
   */
  extern void resume_thread(const uint32_t taskName);

  /**
   * \brief Get the stack usage of this task, in percent
   * \return usage, in [0; 100]
   */
  uint16_t get_usage_percent(const uint32_t taskName);

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

  // Display a threads usage report
  extern void display_thread_debug();

  /// Shutdown the task driver cleanly
  extern void shutdown();

#ifdef __cplusplus
}

} // namespace threads
} // namespace bsp
} // namespace lampda
#endif
