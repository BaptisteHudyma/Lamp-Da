#ifndef INPUTS_H
#define INPUTS_H

namespace inputs {

void init(const bool wasPoweredByUserInterrupt);
void loop();

/// disable custom user modes
void button_disable_usermode();

/// return true if custom user modes are enabled
bool is_button_usermode_enabled();

} // namespace inputs

#endif
