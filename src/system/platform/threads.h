// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_THREADS_H
#define PLATFORM_THREADS_H

#ifdef __cplusplus
extern "C" {
#endif

  // store tasks names here
  const char* const pd_taskName = "usbpd";
  const char* const pdInterruptHandle_taskName = "intpd";
  const char* const power_taskName = "power";
  const char* const user_taskName = "user";
  const char* const taskScheduler_taskName = "task_sched";

  typedef void (*taskfunc_t)(void);
  /**
   * \brief Start a separate thread, running until the system shuts off
   * \param taskFunction the function to run
   * \param[in] taskName The name associated
   * \param[in] priority from  to 2, this thread priority
   * \param[in] stackSize The size of the stack to allocate. can be ignored and checked while running using the command
   * line
   */
  extern void start_thread(taskfunc_t taskFunction,
                           const char* const taskName,
                           const int priority,
                           const int stackSize);
  // start a thread in suspend state
  extern void start_suspended_thread(taskfunc_t taskFunction,
                                     const char* const taskName,
                                     const int priority,
                                     const int stackSize);
  // make this thread pass the control to other threads
  extern void yield_this_thread();

  // threads can only suspend itself
  extern void suspend_this_thread();

  // DANGEROUS: suspend all threads started from here
  extern void suspend_all_threads();

  // resume a target thread
  extern void resume_thread(const char* const taskName);

#ifdef __cplusplus
}
#endif

#endif