#include "output_power.h"

#include <cmath>
#include <cstdint>

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/registers.h"

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
  power::set_output_max_current_mA(5000);
}

void blip()
{
  powergates::power::blip();
  yield_this_thread();
}

} // namespace outputPower
