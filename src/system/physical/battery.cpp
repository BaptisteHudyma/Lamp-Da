#include "battery.h"

#include "src/system/alerts.h"

#include "src/system/utils/constants.h"

#include "src/system/power/charger.h"

#include "src/system/platform/time.h"

namespace battery {

/**
 * \brief Return the battery voltage and raise the battery incoherent alert if needed
 * This will use the GPIO rread at first, and the charger ADC when available
 */
uint16_t get_raw_battery_voltage_mv()
{
  uint16_t batteryVoltage_mV = 0;
  const auto& chargerStates = charger::get_state();
  if (chargerStates.areMeasuresOk)
  {
    // values from the ADC in the charging component
    batteryVoltage_mV = chargerStates.batteryVoltage_mV;
  }
  // else: not ready yet ?
  else
    batteryVoltage_mV = batteryMaxVoltage_mV;

  // in bounds with some margin
  static constexpr uint16_t minInValue = batteryMinVoltage_mV * 0.95;
  static constexpr uint16_t maxInValue =
          batteryMaxVoltage_mV * 1.01; // this should not go over 17v, the max limit we can handle with the current
                                       // hardware (and sampling rate)
  if (batteryVoltage_mV < minInValue or batteryVoltage_mV > maxInValue)
  {
    alerts::manager.raise(alerts::Type::BATTERY_READINGS_INCOHERENT);
    // return a default low value
    return batteryMinVoltage_mV;
  }
  // return the true battery voltage
  alerts::manager.clear(alerts::Type::BATTERY_READINGS_INCOHERENT);
  return batteryVoltage_mV;
}

} // namespace battery
