/*! \file time_handling.hpp
    \brief Define time behavior at a greater scale than platform time.
    Add real time synchronisation.
*/

#ifndef COMPONENT_TIME_HANDLING
#define COMPONENT_TIME_HANDLING

#include <cstdint>

namespace lampda {
namespace component {
/// Handle the platform realtime functions
namespace time {

/// Store a real time reference
struct RealTime
{
  bool is_valid() const
  {
    return dayOfTheWeek >= 0 and dayOfTheWeek < 7 and hour >= 0 && hour < 24 && minutes >= 0 && minutes < 60 &&
           seconds >= 0 && seconds < 60;
  }

  int32_t as_seconds() const;

  // default values are always wrong
  uint8_t dayOfTheWeek = UINT8_MAX; ///< [0; 6] the index of the day of the week. Monday is 0, sunday is 6
  uint8_t hour = UINT8_MAX;         ///< hour of the day, in the [0; 23]
  uint8_t minutes = UINT8_MAX;      ///< minutes, in the [0; 59]
  uint8_t seconds = UINT8_MAX;      ///< seconds, in the [0; 59]
};

/**
 * \brief Set the real time in the lamp, only valid until the next shutdown
 */
bool set_real_time(const RealTime& realTime);

/**
 * \brief Return a real time value, only valid if it was set first.
 */
RealTime get_real_time();

/**
 * \brief Given a real time, return the expected platform time in seconds at wich it will be reached.
 * \warning The returned time in seconds is only valid if the real time was set before.
 * \warning The returned time could be greater than the maximum platform time.
 * \return if the global time is unset, the function return 0
 */
uint32_t get_platform_time_from_target_time(const RealTime& time);

} // namespace time
} // namespace component
} // namespace lampda

#endif
