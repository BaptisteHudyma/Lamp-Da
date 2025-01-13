#include "led_power.h"

#include <cmath>
#include <cstdint>

#include "src/system/utils/constants.h"
#include "src/system/platform/gpio.h"
#include "src/system/utils/utils.h"

namespace ledpower {

/**
 * Power on the current driver with a specific current value
 */
void write_current(const float current)
{
  float currentCurrent = lmpd_constrain(current, 0, maxPowerConsumption_A);

  // map current value to driver value
  const uint8_t mappedDriverValue = utils::map(currentCurrent, 0, maxPowerConsumption_A, 0, 255);

  brigthness_write_analog(mappedDriverValue);
}

/**
 * Power on the current driver with a soecific brightness value
 */
void write_brightness(const uint8_t brightness)
{ // map to current value
  const float brightnessToCurrent = utils::map(brightness, 0, 255, 0, maxStripConsumption_A);
  write_current(brightnessToCurrent);
}

DigitalPin LedPowerPin(DigitalPin::GPIO::a1);

void activate_12v_power()
{
  // power on
  LedPowerPin.set_pin_mode(DigitalPin::Mode::kOutput);
  LedPowerPin.set_high(true);
}

void deactivate_12v_power()
{
  // power off
  LedPowerPin.set_high(false);
  LedPowerPin.set_pin_mode(DigitalPin::Mode::kOutputHighCurrent);
}

} // namespace ledpower
