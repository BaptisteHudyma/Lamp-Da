/*! \file power_handler.h
 *  \brief Entry point of the battery charging and power gate behavior, implemented as a state machine.
 */

#ifndef POWER_POWER_HANDLER_H
#define POWER_POWER_HANDLER_H

#include <cstdint>
#include <string>

namespace lampda {
namespace logic {
/// Main entry point of the power handling of the board.
/// Responsible the main state of the power handling.
namespace power {

/**
 * \brief Call once at system startup.
 * This can fail, and it would set the state to ERROR
 */
void init();
/**
 * \brief Return true if the power handler system is effectivly started.
 * \return False if the init() call failed.
 */
bool is_setup();

/// Return true when power machine finally exits the STARTUP
bool is_started();
/**
 * \brief Return true if the power handling is locked in an ERROR state.
 * No power actions can be made.
 */
bool is_in_error_state();

/**
 * \brief Set the power handler to main OUTPUT_VOLTAGE_MODE
 * \return true if the mode switch is accepted
 */
bool go_to_output_mode();
/**
 * \brief Set the power handler to battery CHARGING_MODE
 * \return true if the mode switch is accepted
 */
bool go_to_charger_mode();
/**
 * \brief Set the power handler to OTG_MODE (eg: external battery mode)
 * \return true if the mode switch is accepted
 */
bool go_to_otg_mode();
/**
 * \brief Set the power handler to IDLE mode (no power output/input)
 * \return true if the mode switch is accepted
 */
bool go_to_idle();
/**
 * \brief Set the power handler to SHUTDOWN mode (last mode before shuting system down)
 * \return true if the mode switch is accepted
 */
bool go_to_shutdown();
/**
 * \brief Set the power handler to ERROR mode (locked safety state)
 * \return true if the mode switch is accepted
 */
bool go_to_error();

/**
 * \brief Set the desired output voltage of the charger.
 *
 * This is just an information, that will be used when the state machine reaches the OUTPUT_VOLTAGE_MODE state.
 * \param[in] outputVoltage_mV Desired output voltage, in millivolts
 */
void set_output_voltage_mv(const uint16_t outputVoltage_mV);
/**
 * \brief Set the desired maximum output current of the charger.
 * This is just an information, that will be used when the state machine reaches the OUTPUT_VOLTAGE_MODE state.
 *
 * \param[in] outputCurrent_mA Desired maximum allowed output current, in milliamps
 */
void set_output_max_current_mA(const uint16_t outputCurrent_mA);

/**
 * \brief Set a new output with a time limit, after wich the output will go back to the original values.
 * It is canceled at any point by a call to set_output_voltage_mv.
 * This is just an information, that will be used when the state machine reaches the OUTPUT_VOLTAGE_MODE state.
 *
 * \param[in] outputVoltage_mV desired temporary output voltage, in millivolts
 * \param[in] outputCurrent_mA desired temporary maximum output current, in milliamps
 * \param[in] timeout_ms Time delay after which the temporary mode will be disabled
 */
void set_temporary_output(const uint16_t outputVoltage_mV, const uint16_t outputCurrent_mA, const uint16_t timeout_ms);

/**
 * \brief Block or allow the charging of the battery.
 * Note that the system can still be in CHARGING_MODE mode if you set this to false, but the battery just wont charge.
 * \return True if accepted
 */
bool enable_charge(const bool);

/**
 * \brief Return the current state, as a string
 */
std::string get_state();
/**
 * \brief Return the current error message, as a string.
 * If not errors are present, will return "x".
 */
std::string get_error_string();

/**
 * \brief Return true when the current state is OUTPUT_VOLTAGE_MODE, and the desired voltage are set and gates are
 * ready. Basically, it returns true when the output is being powered.
 */
bool is_output_mode_ready();

/**
 * \brief Return true if the current state is OUTPUT_VOLTAGE_MODE.
 */
bool is_in_output_mode();
/**
 * \brief Return true if the current state is OTG_MODE.
 */
bool is_in_otg_mode();

} // namespace power
} // namespace logic
} // namespace lampda

#endif
