#ifndef LED_POWER_HPP
#define LED_POWER_HPP

#include "Arduino.h"

namespace ledpower {

void loop();

/** Write a current value directly to the led strip (DANGEROUS)
 * \param[in] current target current, from 0 (off) to maxPowerConsumption_A
 */
extern void write_current(const float current);

/** \brief Write a brightness relative to the led strip used (refer to the
 * constant.h file)
 * \param[in] brightness: 0 to 255 value, that will be converted to a 0 (off) to
 * maxStripConsumption_A
 */
extern void write_brightness(const uint8_t brightness);

extern void write_voltage(const float voltage);

}  // namespace ledpower

#endif