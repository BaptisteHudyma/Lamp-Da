#ifndef INDEXABLE_FUNCTIONS_H
#define INDEXABLE_FUNCTIONS_H

#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "../../system/utils/strip.h"
#include "Arduino.h"

namespace user {

// extern declarations
extern LedStrip strip;

// Called when the system is powered off
// must be non blocking function
void power_on_sequence();

// Called when the system powers off
// must be non blocking function
void power_off_sequence();

// Called when the system changes the led brightness
void brightness_update(const uint8_t brightness);

// read and write parameters to memory
void write_parameters();
void read_parameters();

/** \brief called when button is clicked
 * \param[in] clicks The number of consecutive clicks (>= 2)
 */
void button_clicked(const uint8_t clicks);

/** \brief called when button is clicked and held pressed. It is called while
 * the button is pressed, so holdDuration increases
 * \param[in] clicks The number of consecutive clicks (>= 2)
 * \param[in] isEndOfHoldEvent True when the user releases the button,
 * holdDuration will contain junk
 * \param[in] holdDuration The duration of the held event. (valid if
 * isEndOfHoldEvent is false)
 */
void button_hold(const uint8_t clicks, const bool isEndOfHoldEvent,
                 const uint32_t holdDuration);

// called at each loop call
// if the duration of this call exeeds the loop target time, an alert will be
// raised
void loop();

// if you set this to true, another thread will be spawned, it will call the
// function user_thread
constexpr bool should_spawn_thread() { return true; };

// called by the second thread, activated if should_spawn_thread is true
void user_thread();

}  // namespace user

#endif

#endif