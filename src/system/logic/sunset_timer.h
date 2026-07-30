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

/**
 * \brief Set the sunset timer to a known clock deadline, replacing existing timers. Start the timer, or upate it if
 * needed.
 * \param[in] timeshutdown_s The new time limit. It must be greater than the current time
 */
void set_deadline(const uint32_t timeshutdown_s);

/**
 * \brief add some time to the sunset timer. Limited in the range [1; 10] minutes.
 * If the timer is not started yet, will start it.
 */
void add_time_minutes(const uint8_t time_minutes);

/**
 * \brief If a timer is enabled, shortcut it to the start of the shutdown animation.
 */
void shortcut_to_phaseout();

/**
 * \brief signal to the timer that some time may be added.
 * Only add time if the timer is in the fadeout phase
 */
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
