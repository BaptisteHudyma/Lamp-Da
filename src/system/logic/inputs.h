/*! \file inputs.h
    \brief Handle the logic associated with the input button.
*/

#ifndef INPUTS_H
#define INPUTS_H

namespace lampda {
namespace logic {
/// Handle the system input, the behavior associated to the button inputs.
namespace inputs {

/// Call once on system start
/// \param[in] wasPoweredByUserInterrupt Indicates if the system was powered by the user button interrupt
void init(const bool wasPoweredByUserInterrupt);

/// Call often to handle button updates
void loop();

/// disable custom user modes
void button_disable_usermode();

/// return true if custom user modes are enabled
bool is_button_usermode_enabled();

} // namespace inputs
} // namespace logic
} // namespace lampda

#endif
