/**
 * \file sunrise_timer.h
 * \brief Start the system after a set duration, or a set second
 */

#pragma once

#include <cstdint>

namespace lampda {
namespace logic {
/// Sunrise timer handler
namespace sunrise {

/// Call once on system start
void init();

/**
 * \brief Set the sunrise timer to a known clock deadline, replacing existing timers. Start the timer, or upate it if
 * needed.
 * \param[in] timeToFullyOn_s The new time limit. It must be greater than the current time
 */
void set_deadline(const uint32_t timeToFullyOn_s);

/**
 * \brief add some time to the sunrise timer. Limited in the range [1; 10] minutes.
 * If the timer is not started yet, will start it.
 */
void add_time_minutes(const uint8_t time_minutes);

/// cancel the current active timer
void cancel_timer();

/// True if timer is running
bool is_enabled();

/// Lock the hability of the sunrise timer to control the brightness
void lock_brightness_update(bool shouldLock);

} // namespace sunrise
} // namespace logic
} // namespace lampda
