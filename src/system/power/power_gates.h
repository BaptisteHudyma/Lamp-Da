#ifndef POWER_GATES_H
#define POWER_GATES_H

namespace powergates {

void init();
void loop();

// specific power commands
namespace power {

/**
 * \brief blip power gate (very short power interruption)
 */
void blip();

} // namespace power

/**
 * \brief enable the power gate
 * This will first disable the vbus gate, then after a delay enable the power gate
 */
void enable_power_gate();

/**
 * \brief return true when the gate if enabled (after the delay)
 */
bool is_power_gate_enabled();

/**
 * \brief enable the vbus gate
 * This will first disable the power gate, then after a delay enable the vbus gate
 */
void enable_vbus_gate();
/**
 * \brief Fast swap only: diretcly close output gate and enable vbus gate
 */
void enable_vbus_gate_DIRECT();

/**
 * \brief return true when the gate if enabled (after the delay)
 */
bool is_vbus_gate_enabled();

/**
 * \brief disable immediatly all gates, stop the unlocking processes
 */
void disable_gates();

/**
 * \brief return true if the gates are disabled
 */
bool are_gate_disabled();

} // namespace powergates

#endif