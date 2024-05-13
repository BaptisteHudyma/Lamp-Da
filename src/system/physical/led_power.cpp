#include "led_power.h"

#include "../utils/constants.h"
#include "../utils/utils.h"

namespace ledpower {

/**
 * Power on the current driver with a soecific current value
 */
void write_current(const float current) {
  // map current value to driver value
  const uint8_t mappedDriverValue =
      utils::map(constrain(current, 0, maxPowerConsumption_A), 0,
                 maxPowerConsumption_A, 0, 255);

  analogWrite(OUT_BRIGHTNESS, mappedDriverValue);
}

/**
 * Power on the current driver with a soecific brightness value
 */
void write_brightness(const uint8_t brightness) {  // map to current value
  const float brightnessToCurrent =
      utils::map(brightness, 0, 255, 0, maxStripConsumption_A);
  write_current(brightnessToCurrent);
}

}  // namespace ledpower