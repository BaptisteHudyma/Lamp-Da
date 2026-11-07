#include "time_handling.h"

#include "src/system/hal/time.h"
#include <cstdint>

namespace lampda {
namespace component {

namespace time {

static constexpr uint32_t secondsPerMinutes = 60;
static constexpr uint32_t MinutesPerHours = 60;
static constexpr uint32_t hoursPerDays = 24;
static constexpr uint32_t daysPerWeeks = 7;

static constexpr uint32_t secondsPerHours = secondsPerMinutes * MinutesPerHours;
static constexpr uint32_t secondsPerDays = secondsPerHours * hoursPerDays;
static constexpr uint32_t secondsPerWeeks = secondsPerDays * daysPerWeeks;

int32_t RealTime::as_seconds() const
{
  return dayOfTheWeek * secondsPerDays + hour * secondsPerHours + minutes * secondsPerMinutes + seconds;
}

namespace internal {

/**
 * The internal time starts at zero every time the system starts, so the time_offset_s should just be added to the
 * current lamp time to obtain real time.
 * The internal clock can drift but this data is not qualified.
 * The internal clock resets to zero every 49 days of activity, which cannot realistically be reached.
 */

bool is_time_offset_set = false;
/// Time offset, in seconds, to the internal lamp time. It can be positive or negative.
/// It is defined relative to the first second of the week.
int32_t real_time_offset_s = 0;

/// Return the real time in seconds
int32_t get_real_time_seconds(const int32_t platformTimeS, const int32_t timeOffsetS)
{
  if (not is_time_offset_set)
    return INT32_MAX;
  return platformTimeS - timeOffsetS;
}

/// Compute the lamp real time
RealTime get_real_time(const int32_t platformTimeS, const int32_t timeOffsetS)
{
  RealTime time;
  if (not is_time_offset_set)
    return time;

  // time since the lamp started
  const int32_t timeS = get_real_time_seconds(platformTimeS, timeOffsetS);

  const int32_t currentDay = timeS / secondsPerDays;
  const int32_t secondsLeftInTheDay = timeS % secondsPerDays;

  const int32_t currentHour = secondsLeftInTheDay / secondsPerHours;
  const int32_t secondsLeftInTheHour = secondsLeftInTheDay % secondsPerHours;

  const int32_t currentMinute = secondsLeftInTheHour / secondsPerMinutes;
  const int32_t secondsLeftInTheMinute = secondsLeftInTheHour % secondsPerMinutes;

  time.dayOfTheWeek = currentDay % 7;
  time.hour = currentHour;
  time.minutes = currentMinute;
  time.seconds = secondsLeftInTheMinute;
  return time;
}

} // namespace internal

bool set_real_time(const RealTime& realTime)
{
  if (not realTime.is_valid())
    return false;

  internal::real_time_offset_s = hal::time_s() - realTime.as_seconds();
  internal::is_time_offset_set = true;
  return true;
}

RealTime convert_to_real_time(const uint32_t time_s)
{
  return internal::get_real_time(time_s, internal::real_time_offset_s);
}

RealTime get_real_time() { return internal::get_real_time(hal::time_s(), internal::real_time_offset_s); }

uint32_t get_platform_time_from_target_time(const RealTime& time)
{
  if (not internal::is_time_offset_set)
  {
    return 0;
  }
  const int64_t now_s = hal::time_s();
  // platform time of the target in the week of the time synchronization, in seconds
  const int64_t projectedPlatformTime_s = static_cast<int64_t>(time.as_seconds()) + internal::real_time_offset_s;

  // the target is the next occurrence of this day and time: move it to the current week, then to the next week if it
  // is already in the past. The current time itself is kept as is.
  int64_t timeUntilTarget_s = (projectedPlatformTime_s - now_s) % static_cast<int64_t>(secondsPerWeeks);
  if (timeUntilTarget_s < 0)
    timeUntilTarget_s += secondsPerWeeks;

  return static_cast<uint32_t>(now_s + timeUntilTarget_s);
}

int8_t get_next_enabled_day(const uint8_t daysMask, const uint8_t hour, const uint8_t minutes, const uint8_t seconds)
{
  int8_t bestDay = -1;
  uint32_t bestTime_s = 0;
  for (uint8_t day = 0; day < 7; ++day)
  {
    if ((daysMask & (1u << day)) == 0)
      continue;

    RealTime time;
    time.dayOfTheWeek = day;
    time.hour = hour;
    time.minutes = minutes;
    time.seconds = seconds;
    const uint32_t time_s = get_platform_time_from_target_time(time);
    // without a real time, every day gives 0: keep the first enabled one
    if (bestDay < 0 or time_s < bestTime_s)
    {
      bestDay = day;
      bestTime_s = time_s;
    }
  }
  return bestDay;
}

} // namespace time
} // namespace component
} // namespace lampda
