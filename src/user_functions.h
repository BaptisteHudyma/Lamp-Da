#ifndef SYSTEM_BASE_H
#define SYSTEM_BASE_H

#include "Arduino.h"
#include "system/utils/strip.h"

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

/** \brief called when button is clicked, whenever "default UI" is active.
 *
 * \param[in] clicks The number of consecutive clicks
 */
void button_clicked_default(const uint8_t clicks);

/** \brief called when button is clicked then held pressed. It is called
 *         repeatedly while the button is pressed, with holdDuration
 *         increasing. This is called when "default UI" is active.
 *
 * \param[in] clicks Number of clicks including the long-pressed one
 * \param[in] isEndOfHoldEvent True if the user released the button
 * \param[in] holdDuration The duration of the held event
 *
 * Note that if isEndOfHoldEvent is True, then holdDuration is zero.
 */
void button_hold_default(const uint8_t clicks,
                         const bool isEndOfHoldEvent,
                         const uint32_t holdDuration);

/** \brief called when button is clicked, if "usermode UI" is active. If an
 *         action was performed, it must return true, if not (returns false)
 *         the "default UI" action is performed.
 *
 * \param[in] clicks The number of consecutive clicks
 */
bool button_clicked_usermode(const uint8_t clicks);

/** \brief called when button is clicked then held pressed, if "usermode UI" is
 *         active. It is called repeatedly with holdDuration increasing. If an
 *         action was performed, it must return true, if not (returns false)
 *         the "default UI" action is performed.
 *
 * \param[in] clicks Number of clicks including the long-pressed one
 * \param[in] isEndOfHoldEvent True when the user releases the button
 * \param[in] holdDuration The duration of the held event
 *
 * Note that if isEndOfHoldEvent is True, then holdDuration is zero.
 */
bool button_hold_usermode(const uint8_t clicksBeforeLastHold,
                          const bool isEndOfHoldEvent,
                          const uint32_t holdDuration);

// called at each loop call
// if the duration of this call exeeds the loop target time, an alert will be
// raised
void loop();

// if you set this to true, another thread will be spawned, it will call the
// function user_thread
bool should_spawn_thread();

// called by the second thread, activated if should_spawn_thread is true
void user_thread();

}  // namespace user

#endif
