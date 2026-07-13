/** \file
 *  \brief Basic controller behavior, including alerts and user interactions
 */

#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include "src/compile.h"
#include <cstdint>
#include <string>

namespace lampda {
namespace logic {
/// Main system behavior handle
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

/// Return true if the system is expected to power on.
bool is_system_should_be_powered();

/// set system state to "output on"
void set_power_on();
/// set system state to "output off". Can be ignored
void set_power_off();

/// Try to switch to external battery mode
void go_to_external_battery_mode();

/// return true if the system is in start mode
bool is_in_start_logic_state();
/// return true if the system is in output mode
bool is_in_output_state();
/// return true if the system is in charge mode
bool is_in_charge_state();

namespace internal {

/**
 * \brief DO NOT USE: shutdown the lamp immediatly. only for emmergency/special cases
 * \param[in] shouldSaveUserParameters If False, no user parameters will be written to memory
 * \param[in] shouldSaveSystemParameters If False, no system parameters will be written to memory
 */
void handle_shutdown_state(const bool shouldSaveUserParameters = true, const bool shouldSaveSystemParameters = true);

/// return the remaining bluetooth auto activations
uint32_t get_bluetooth_auto_activation_left();

} // namespace internal

namespace sunset {
/// signal to behavior the advance of the sunset timer
void progress_update(const float progress);
} // namespace sunset

} // namespace behavior
} // namespace logic
} // namespace lampda

/// \private Internal symbol used to signify which LMBD_LAMP_TYPE was specified
#ifdef LMBD_CPP17
extern const char* ensure_build_canary();
#endif

#endif
