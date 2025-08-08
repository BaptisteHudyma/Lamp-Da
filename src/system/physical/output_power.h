#ifndef OUTPUT_POWER_HPP
#define OUTPUT_POWER_HPP

#include <cstdint>
namespace outputPower {

/** \brief Write a voltage to the output (will only write it in output mode)
 * \param[in] voltage_mv: 0 to 20000 mV
 */
extern void write_voltage(const uint16_t voltage_mv);

/**
 * \brief Very short interruption of output voltage
 */
extern void blip();

/**
 * \brief close all external voltage path
 */
void disable_power_gates();

} // namespace outputPower

#endif