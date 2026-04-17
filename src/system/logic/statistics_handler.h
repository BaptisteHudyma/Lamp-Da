/*! \file statistics_handler.h
    \brief Handle the loading and storing of the system statistics.
*/

#ifndef LOGIC_STATISTICS_HANDLER_H
#define LOGIC_STATISTICS_HANDLER_H

#include <cstdint>

/// Handle update and tracking of user use statistics
namespace statistics {

/**
 * \brief Load the statistics from the slash memeory
 */
void load_from_memory();
/**
 * \brief Write statistics to the system flash memory
 */
void write_to_memory();

/**
 * \brief Signal that a button press was detected
 */
void signal_button_press();

/**
 * \brief Signal that the output just turned on.
 */
void signal_output_on();
/**
 * \brief Signal that the output just turned off.
 */
void signal_output_off();

/**
 * \brief Signal that the battery is charging.
 */
void signal_battery_charging_on();
/**
 * \brief Signal that the battery is not charging.
 */
void signal_battery_charging_off();

/**
 * \brief Signal that an alert of any type was raised.
 *\param[in] alertMask bitwise mask of raised alerts
 */
void signal_alert_raised(uint32_t alertMask);

/**
 * \brief Debug the statistics to serial output.
 * \param[in] shouldShowAlerts If true, the alerts statistics will be displayed.
 */
void show(const bool shouldShowAlerts = true);

} // namespace statistics

#endif
