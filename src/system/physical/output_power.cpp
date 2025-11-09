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

  power::set_output_voltage_mv(lmpd_constrain<uint32_t>(voltage_mv, 0, 20000));
  power::set_output_max_current_mA(3000);
}

void write_temporary_output_limits(const uint16_t voltage_mv, const uint16_t current_ma, const uint32_t timeout_ms)
{
  const uint32_t realTimeout_ms = min<uint32_t>(5000, timeout_ms);
  //
  power::set_temporary_output(voltage_mv, current_ma, realTimeout_ms);
}

void blip(const uint32_t timing) { powergates::power::blip(timing); }

void disable_power_gates() { powergates::disable_gates(); }

} // namespace outputPower
