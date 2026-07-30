#include "time_handling.h"

#include "src/system/platform/time.h"
#include <cstdint>

namespace lampda {
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

  internal::real_time_offset_s = platform::time_s() - realTime.as_seconds();
  internal::is_time_offset_set = true;
  return true;
}

RealTime get_real_time() { return internal::get_real_time(platform::time_s(), internal::real_time_offset_s); }

uint32_t get_platform_time_from_target_time(const RealTime& time)
{
  if (not internal::is_time_offset_set)
  {
    return 0;
  }
  // get the expected platform time to reach the given time, in seconds
  const int32_t projectedPlatformTime_s = time.as_seconds() + internal::real_time_offset_s;

  /// TODO: handle the clock wrap around

  // if time is negative, we will expect that the given time will be in the future, next week
  if (projectedPlatformTime_s < 0)
  {
    // +1 to go positive, +1 to go to next week and not stay in the same week
    return secondsPerWeeks + projectedPlatformTime_s * 2;
  }
  return projectedPlatformTime_s;
}

} // namespace time
} // namespace lampda
