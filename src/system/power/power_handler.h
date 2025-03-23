#ifndef POWER_POWER_HANDLER_H
#define POWER_POWER_HANDLER_H

#include <cstdint>
#include <string>
namespace power {

// call before all
void init();

// call often
void loop();

// control state change
bool go_to_output_mode();
bool go_to_charger_mode();
bool go_to_otg_mode();
bool go_to_shutdown();

// control special commands for every states
// output mode
bool set_output_voltage_mv(const uint16_t);
bool set_output_max_current_mA(const uint16_t);

// charge mode
bool enable_charge(const bool);

// otg mode
bool set_otg_voltage_mv(const uint16_t);
bool set_otg_max_current_mA(const uint16_t);

std::string get_state();

bool is_in_output_mode();

} // namespace power

#endif