/*! \file sunset_timer.h
    \brief Logic of the sunset time, eg the system auto stops after a set delay.
*/

#ifndef UTILS_SUNSET_TIMER_H
#define UTILS_SUNSET_TIMER_H

#include <cstdint>

namespace lampda {
namespace logic {
/// Sunset timer handler
namespace sunset {

/// call once on program start
void init();

/// add some time to the sunset timer. Limited to 10 minutes
void add_time_minutes(const uint8_t time_minutes);

/// Set the sunset timer to a known clock deadline, replacing existing timers
void set_deadline(const uint32_t timeshutdown_s);

/// signal to the timer that some time may be added
void bump_timer();

/// cancel the current active timer
void cancel_timer();

/// True if timer is running
bool is_enabled();

/// Lock the hability of the sunset timer to control the brightness
void lock_brightness_update(bool shouldLock);

} // namespace sunset
} // namespace logic
} // namespace lampda

#endif
