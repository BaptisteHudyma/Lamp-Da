#include "led_power.h"

#include <cmath>
#include <cstdint>

#include "../utils/constants.h"
#include "../utils/utils.h"

namespace ledpower {

/**
 * Power on the current driver with a specific current value
 */
void write_current(const float current) {
  float currentCurrent = constrain(current, 0, maxPowerConsumption_A);

  // map current value to driver value
  const uint8_t mappedDriverValue =
      utils::map(currentCurrent, 0, maxPowerConsumption_A, 0, 255);

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