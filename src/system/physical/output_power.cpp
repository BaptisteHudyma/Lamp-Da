#include "output_power.h"

#include <cmath>
#include <cstdint>

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/power/power_handler.h"
#include "src/system/power/power_gates.h"

namespace outputPower {

/**
 * Power on the current driver with a specific brightness value
 */
void write_voltage(const uint16_t voltage_mv)
{ // map to current value
  if (voltage_mv == 0)
  {
    power::set_output_voltage_mv(0);
    power::set_output_max_current_mA(0);
    return;
  }

  power::set_output_voltage_mv(lmpd_constrain(voltage_mv, 0, 20000));
  power::set_output_max_current_mA(3000);
}

void blip(const uint32_t timing) { powergates::power::blip(timing); }

void disable_power_gates() { powergates::disable_gates(); }

} // namespace outputPower
