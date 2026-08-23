/*! \file threads.h
    \brief Interface for the platform specific tasks and threads.
*/

// do not use pragma once here, has this can be mocked
#ifndef HAL_THREADS_H
#define HAL_THREADS_H

#ifdef __cplusplus

#include <cstdint>

namespace lampda {
namespace hal {
/// Define tasks and threads specifics.
namespace threads {

extern "C" {
#endif

  typedef void (*taskfunc_t)(void);
  typedef void* TaskHandle_t;

  /**
   * \brief Create a thread from the given function and return the associated handle
   * \brief[in] handle Handle that will be allocated by the thread
   * \param[in] task Task function to execute. it will be called in a loop
   * \param[in] name internal name of the thread
   * \param[in] priority from  to 2, this thread priority
   * \param[in] stackSize The size of the stack to allocate.
   * \param[in] startSuspended if == 0, start the thread in a suspended mode
   * \return 0 for success, anything else is failure
   */
  int HAL_create_thread(TaskHandle_t* const handle,
                        taskfunc_t task,
                        char const* const name,
                        const int priority,
                        const int stackSize,
                        const int startSuspended);

  /// Yield this thread
  void HAL_yield();

  /// Suspend this thread
  void HAL_suspend();

  /**
   * \brief Put this thread in suspended mode, waiting for a notification. This function wil block execution until the
   timeout is reached or a value is received.
   * \param[in] timeout_ms Timeout delay. Can be zero of less for no timeout
   * \return the value that woke up this thread
   */
  int HAL_wait_notification(const int timeout_ms);

  /// Notify a thread with a value
  void HAL_notify_thread(TaskHandle_t handle, int wakeUpEvent);

  /// Suspend the target thread
  void HAL_suspend_thread(TaskHandle_t handle);

  /// Return 0 if thread is suspended
  int HAL_is_suspended(TaskHandle_t handle);

  /// Resume a suspended thread
  void HAL_resume_thread(TaskHandle_t handle);

  /// Given a buffer, return a buffer debug text
  void HAL_get_debug_thread_text(char* textBuff);

  /// Shutdown the thread HAL
  void HAL_shutdown();

#ifdef __cplusplus
}

} // namespace: threads
} // namespace: hal
} // namespace: lampda
#endif

#endif
