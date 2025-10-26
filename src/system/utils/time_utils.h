#ifndef TIME_UTILS_HPP
#define TIME_UTILS_HPP

#include <cstdint>

#include "src/system/platform/time.h"

// define time routines
class CEveryNMillis
{
public:
  uint32_t mPrevTrigger;
  uint32_t mPeriod;
  bool isInit;

  CEveryNMillis()
  {
    isInit = false;
    reset();
    mPeriod = 1;
  };
  CEveryNMillis(uint32_t period)
  {
    isInit = false;
    reset();
    setPeriod(period);
  };
  void setPeriod(uint32_t period) { mPeriod = period; };
  uint32_t getTime() const { return time_ms(); };
  uint32_t getPeriod() const { return mPeriod; };
  uint32_t getElapsed() const { return getTime() - mPrevTrigger; }
  uint32_t getRemaining() const { return mPeriod - getElapsed(); }
  uint32_t getLastTriggerTime() const { return mPrevTrigger; }
  bool ready()
  {
    bool isReady = not isInit or (getElapsed() >= mPeriod);
    if (isReady)
    {
      isInit = true;
      reset();
    }
    return isReady;
  }
  void reset() { mPrevTrigger = getTime(); };
  void trigger() { mPrevTrigger = getTime() - mPeriod; };

  operator bool() { return ready(); }
};

#define EVERY_N_MILLIS_I(NAME, N) \
  static CEveryNMillis NAME(N);   \
  if (NAME)

#define EVERY_N_MILLIS_REFRESH_I(NAME, N) \
  static CEveryNMillis NAME(N);           \
  NAME.setPeriod(N);                      \
  if (NAME)

#define EVERY_N_MILLIS_WITH_COND_I(NAME, COND, N) \
  static CEveryNMillis NAME(N);                   \
  if (COND || NAME)

// call the following block of code every N milliseconds
#define EVERY_N_MILLIS(N) EVERY_N_MILLIS_I(PER##__COUNTER__, N)
// call the following block of code every N milliseconds, with a possible update to the delay
#define EVERY_N_MILLIS_REFRESH(N) EVERY_N_MILLIS_REFRESH_I(PER##__COUNTER__, N)
// call the following block of code every N milliseconds, with an alternative trigge boolean
#define EVERY_N_MILLIS_COND(N, COND) EVERY_N_MILLIS_WITH_COND_I(PER##__COUNTER__, COND, N)

#endif
