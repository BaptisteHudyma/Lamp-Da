#ifndef USER_FUNCTIONS_H
#define USER_FUNCTIONS_H

/** \file functions.h
 *  \brief Custom user mode functions for indexable strips
 **/

#include "Arduino.h"

//
// code specific to LMBD_LAMP_TYPE=indexable
//

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "src/system/utils/strip.h"

// (extern declarations)
namespace user {
extern LedStrip strip;
}
#endif

//
// code common to all lamp types
//

/// Contains code handling custom user mode functions for indexable strips
namespace user {

/// Called when the system powers on (must be non blocking function!)
void power_on_sequence();

/// Called when the system powers off (must be non blocking function!)
void power_off_sequence();

/** \brief Called when the system changes the LED strip brightness
 *
 * \param[in] brightness The brightness value set by the system
 * \remark Use update_brightness() to change brightness in order for
 * brightness_update() to be correctly called on brightness change
 */
void brightness_update(const uint8_t brightness);

/// Called when system wants to write parameters to filesystem
void write_parameters();

/// Called when system wants to read parameters from filesystem
void read_parameters();

/** \brief Called to handle button click events for default user mode behaviors
 *
 * Default behavior is the "default UI" navigation mode, that supports:
 *  - 2 clicks: next user mode
 *  - 3 clicks: next mode group
 *  - 4 clicks: jump to favorite
 *
 * Event 1C/7C+ (shutdown) is handled by
 * button_clicked_callback() in behavior.h
 *
 * \param[in] nbClick The number of clicks made by the user
 */
void button_clicked_default(const uint8_t clicks);

/** \brief Called to handle button click+hold events for user mode behaviors
 *
 * Default behavior is the "default UI" navigation mode, that supports:
 *  - 3 click+hold: configure custom user ramp
 *  - 4 click+hold: scroll across modes and groups
 *  - 5 click+hold (3s): configure favorite modes
 *
 * Event 1H/2H (brightness) is handled by
 * button_hold_callback() in behavior.h
 *
 * \param[in] nbClickAndHold The number of clicks made by the user
 * \param[in] isEndOfHoldEvent True if the user just released the button
 * \param[in] holdDuration The duration of the on-going held event
 * \remark When \p isEndOfHoldEvent is True, then \p holdDuration is zero
 */
void button_hold_default(const uint8_t nbClickAndHold, const bool isEndOfHoldEvent, const uint32_t holdDuration);

/** \brief Called to handle button click events if "usermode UI" is active
 *
 * \param[in] nbClick The number of clicks made by the user
 * \return Should return True if default UI action should be prevented
 * \remark Handler should return False, except if a custom action has been
 * performed, or else the lamp may appear unresponsive to the user
 */
bool button_clicked_usermode(const uint8_t nbClick);

/** \brief Called to handle button click+hold events if "usermode UI" is on
 *
 * \param[in] nbClickAndHold The number of clicks made by the user
 * \param[in] isEndOfHoldEvent True if the user just released the button
 * \param[in] holdDuration The duration of the on-going hold event
 * \return Should return True if default UI action should be prevented
 * \remark When \p isEndOfHoldEvent is True, then \p holdDuration is zero
 */
bool button_hold_usermode(const uint8_t nbClickAndHold, const bool isEndOfHoldEvent, const uint32_t holdDuration);

/** \brief Called at each tick of the main loop
 *
 * By default, dispatched to modes::BasicMode::loop()
 */
void loop();

/** \brief Determine if a second user_thread() loop thread should be spawned
 *
 * \return Return True if user_thread() shall be executed in another thread
 */
bool should_spawn_thread();

/** \brief Called at each tick of the secondary thread
 *
 * \remark This loop is only if should_spawn_thread() returned
 * True when the system probed it to check if it needed a secondary thread
 */
void user_thread();

} // namespace user

#endif
