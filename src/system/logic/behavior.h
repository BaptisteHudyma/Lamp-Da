#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

/** \file
 *
 *  \brief Basic controller behavior, including alerts and user interactions
 **/

#include "src/compile.h"
#include <string>

namespace behavior {

/**
 * \brief Load the parameters from the filesystem
 * \return True is the parameters where loaded
 */
extern bool read_parameters();

/**
 * \return true if the user code is runing
 */
extern bool is_user_code_running();

/**
 * \brief call once at program start, before the first main loop runs
 * The system was powered from a vbus powered event, not a user action
 */
extern void set_woke_up_from_vbus(const bool wokeUp);

// main loop of the system
extern void loop();

std::string get_state();

/// return the error message associated with the error state
std::string get_error_state_message();

/// true if system can work at all (charger or output mode)
bool can_system_allowed_to_be_powered();

/// set system state to "output on"
void set_power_on();
/// set system state to "output off". Can be ignored
void set_power_off();

//
void go_to_external_battery_mode();

} // namespace behavior

/// \private Internal symbol used to signify which LMBD_LAMP_TYPE was specified
#ifdef LMBD_CPP17
extern const char* ensure_build_canary();
#endif

#endif
