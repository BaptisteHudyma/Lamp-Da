#ifndef POWER_POWER_HANDLER_H
#define POWER_POWER_HANDLER_H

#include <cstdint>
#include <string>
namespace power {

// call before all
void init();
void start_threads();
bool is_setup();

// call often
void loop();

// control state change
bool go_to_output_mode();
bool go_to_charger_mode();
bool go_to_otg_mode();
bool go_to_idle();
bool go_to_shutdown();

// control special commands for every states
// output mode
bool set_output_voltage_mv(const uint16_t);
bool set_output_max_current_mA(const uint16_t);

// charge mode
bool enable_charge(const bool);

std::string get_state();
std::string get_error_string();

bool is_output_mode_ready();

bool is_in_output_mode();
bool is_in_otg_mode();

} // namespace power

#endif