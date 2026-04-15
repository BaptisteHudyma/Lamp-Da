/*! \file power_gates.h
    \brief Interface layer that handles the safety of the power gaters of the system.

    The power gates are the USB gate, to let power pass between the charger and the USB (bidirectional),
    and the output gate, to send power to the board main output.
*/

#ifndef POWER_GATES_H
#define POWER_GATES_H

#include <cstdint>

/// Handle the two main power gates : Output and USB.
namespace powergates {

/// Called once on system startup
void init();
/// Called often to refresh the status
void loop();

/// specific power commands
namespace power {

/**
 * \brief blip output gate (very short power interruption)
 * \param[in] timing a timing in milliseconds.
 */
void blip(const uint32_t timing);

/// Cancel the current blip if any
void cancel_blip();

/// Return true if the output gate is in a blip
bool is_bliping();

} // namespace power

/**
 * \brief enable the power gate.
 * This will first disable the vbus gate, then after a delay enable the power gate.
 */
void enable_power_gate();

/**
 * \brief return true when the gate if enabled (after the delay).
 */
bool is_power_gate_enabled();

/**
 * \brief enable the vbus gate.
 * This will first disable the power gate, then after a delay enable the vbus gate.
 */
void enable_vbus_gate();
/**
 * \brief Fast swap only: directly close output gate and enable vbus gate.
 */
void enable_vbus_gate_DIRECT();

/**
 * \brief return true when the gate if enabled (after the delay).
 */
bool is_vbus_gate_enabled();

/**
 * \brief disable immediatly all gates, stop the unlocking processes.
 */
void disable_gates();

/**
 * \brief return true if the gates are disabled.
 */
bool are_gate_disabled();

} // namespace powergates

#endif
