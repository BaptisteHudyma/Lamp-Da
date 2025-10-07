#ifndef LOGIC_STATISTICS_HANDLER_H
#define LOGIC_STATISTICS_HANDLER_H

/**
 * Handle update and tracking of user use statistics
 */
#include <cstdint>
namespace statistics {

/**
 * \brief load from memory
 */
void load_from_memory();
/**
 * \brief write to memory (before turn off)
 */
void write_to_memory();

/**
 * \brief signal a button press
 */
void signal_button_press();

/**
 * \brief output usage statistic
 */
void signal_output_on();
void signal_output_off();

/**
 * \brief battery charge statistic
 */
void signal_battery_charging_on();
void signal_battery_charging_off();

/**
 * Display statistics on output
 */
void show();

} // namespace statistics

#endif
