#ifndef POWER_POWER_HANDLER_H
#define POWER_POWER_HANDLER_H

#include <cstdint>
#include <string>
namespace power {

// call before all
void init();
void start_threads();
bool is_setup();

// if this returns true, do nothing
bool is_in_error_state();

// control state change
bool go_to_output_mode();
bool go_to_charger_mode();
bool go_to_otg_mode();
bool go_to_idle();
bool go_to_shutdown();
bool go_to_error();

// control special commands for every states
// output mode
void set_output_voltage_mv(const uint16_t);
void set_output_max_current_mA(const uint16_t);
/**
 * \brief Set a new output with a time limit, after wich the output will go back to the original values
 * It is canceled at any point by a call to set_output_voltage_mv
 */
void set_temporary_output(const uint16_t outputVoltage_mV, const uint16_t outputCurrent_mA, const uint16_t timeout_ms);

// charge mode
bool enable_charge(const bool);

std::string get_state();
std::string get_error_string();

bool is_output_mode_ready();

bool is_in_output_mode();
bool is_in_otg_mode();

bool is_in_external_battery_mode();

} // namespace power

#endif
