#ifndef UTILS_SUNSET_TIMER_H
#define UTILS_SUNSET_TIMER_H

#include <cstdint>

namespace sunset {

// call once on program start
void init();

/// add some time to the sunset timer. Limited to 10 minutes
void add_time_minutes(const uint8_t time_minutes);

/// signal to the timer that some time may be added
void bump_timer();

/// cancel the current active timer
void cancel_timer();

} // namespace sunset

#endif
