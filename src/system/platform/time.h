// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_TIME
#define PLATFORM_TIME

#ifdef __cplusplus
extern "C" {
#endif

  /*
   *  Define all time related platform functions
   */

#include <stdint.h>

  /**
   * \brief Returns the number of milliseconds since the Arduino board began running the current program.
   * This number will overflow (go back to zero), after approximately 50 days.
   *
   * \return Number of milliseconds since the program started (uint32_t)
   */
  extern uint32_t time_ms(void);

  /**
   * \brief Returns the number of microseconds since the Arduino board began running the current program.
   *
   * This number will overflow (go back to zero), after approximately 70 minutes. On 16 MHz Arduino boards
   * (e.g. Duemilanove and Nano), this function has a resolution of four microseconds (i.e. the value returned is
   * always a multiple of four). On 8 MHz Arduino boards (e.g. the LilyPad), this function has a resolution
   * of eight microseconds.
   *
   * \note There are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second.
   */
  extern uint32_t time_us(void);

  /**
   * \brief Pauses the program for the amount of time (in miliseconds) specified as parameter.
   * (There are 1000 milliseconds in a second.)
   *
   * \param dwMs the number of milliseconds to pause (uint32_t)
   */
  extern void delay_ms(uint32_t dwMs);

  /**
   * \brief Pauses the program for the amount of time (in microseconds) specified as parameter.
   *
   * \param dwUs the number of microseconds to pause (uint32_t)
   */
  extern void delay_us(uint32_t dwUs);

  // help functions
  inline uint32_t time_s() { return time_ms() / 1000; }
  inline uint32_t time_min() { return time_s() / 60; }

  inline void delay_s(uint32_t s) { delay_ms(s * 1000); }

#ifdef __cplusplus
}
#endif

#endif
