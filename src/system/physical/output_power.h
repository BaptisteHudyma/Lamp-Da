#ifndef OUTPUT_POWER_HPP
#define OUTPUT_POWER_HPP

#include <cstdint>
namespace outputPower {

/**
 * \brief Write a voltage to the output (will only write it in output mode)
 * \param[in] voltage_mv: 0 to 20000 mV
 */
extern void write_voltage(const uint16_t voltage_mv);

/**
 * \brief overwrite the output characteristics for a given duration
 * Gets back to the previous settings after the tiemout, or if write_voltage is called again
 * \param[in] voltage_mv: 0 to 20000 mV
 * \param[in] current_ma: 0 to 6000mA
 * \param[in] timeout_ms: 0 to 5000mS
 */
extern void write_temporary_output_limits(const uint16_t voltage_mv,
                                          const uint16_t current_ma,
                                          const uint32_t timeout_ms);

/**
 * \brief Very short interruption of output voltage
 */
extern void blip(const uint32_t timing);

/**
 * \brief close all external voltage path
 */
void disable_power_gates();

} // namespace outputPower

#endif
