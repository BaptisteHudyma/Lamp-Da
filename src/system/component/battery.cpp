#include "battery.h"

#include "src/system/logic/alerts.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/time_utils.h"

#include "src/system/component/charger.h"

#include "src/system/bsp/balancer.h"

namespace lampda {
namespace component {
namespace battery {

// no battery present after this timing will raise the alert
static constexpr uint32_t noBatteryAlertRaiseTiming_ms = 1500;

static uint16_t s_batteryVoltage_mV = 0;

/**
 * \brief check the battery pack voltage, and individual cell voltages
 * This should be the main used function.
 * \return false in case of a problem
 */
bool check_balancer_battery_voltage()
{
  const auto& balancerStatus = bsp::balancer::get_status();
  if (balancerStatus.is_valid())
  {
    s_batteryVoltage_mV = balancerStatus.stackVoltage_mV;
    return true;
  }
  return false;
}

/**
 * \brief check the battery voltage from charging component
 */
bool check_charger_battery_voltage()
{
  const auto& chargerStates = component::charger::get_state();
  if (chargerStates.areMeasuresOk)
  {
    // values from the ADC in the charging component
    s_batteryVoltage_mV = chargerStates.batteryVoltage_mV;
    return true;
  }
  return false;
}

/**
 * \brief Return the battery voltage and raise the battery incoherent alert if needed.
 */
uint16_t get_raw_battery_voltage_mv()
{
  // absolute minimum/maximum battery pack voltage
  static constexpr uint16_t minBatteryVoltage_mV =
          physical_constants::minSingularBatteryVoltage_mV * physical_constants::batterySeriesCount;
  static constexpr uint16_t maxBatteryVoltage_mV =
          physical_constants::maxSingularBatteryVoltage_mV * physical_constants::batterySeriesCount;

  // WILL UPDATE s_batteryVoltage_mV VALUE if they return true, so order of the call matters.
  // The last call will old the true stack value, so prefer balancer that is a dedicated more precise component.
  const bool isChargerCheckValid = check_charger_battery_voltage();
  const bool isBalancerCheckValid = check_balancer_battery_voltage();
  if (not(isBalancerCheckValid and isChargerCheckValid))
  {
    // else: not ready yet ? error, return max voltage for now
    // after a set time, return an error, the system should not be used without batteries
    // Probable causes:
    // - No battery connected
    // - A battery wire came loose
    // - A battery is destroyed (end of life)
    // - Balancer or charger component died
    if (hal::time_ms() > noBatteryAlertRaiseTiming_ms)
    {
      logic::alerts::manager.raise(logic::alerts::Type::BATTERY_MISSING);
    }
    return maxBatteryVoltage_mV;
  }

  // check battery pack validity (in bounds with some margin)
  if (
          // minimum limit is not a hard stop, battery can be deeply discharged
          // s_batteryVoltage_mV < minBatteryVoltage_mV or
          s_batteryVoltage_mV > maxBatteryVoltage_mV)
  {
    logic::alerts::manager.raise(logic::alerts::Type::BATTERY_READINGS_INCOHERENT);
    // return a default value
    return 0;
  }
  // return the true battery voltage
  return s_batteryVoltage_mV;
}

bool is_battery_usable_as_power_source() { return logic::alerts::manager.can_use_output_power(); }

bool can_battery_be_charged() { return logic::alerts::manager.can_charge_battery(); }

uint16_t get_battery_minimum_cell_level()
{
  uint16_t minCellVoltage = physical_constants::maxSingularBatteryVoltage_mV;
  const auto& balancerStatus = bsp::balancer::get_status();
  if (balancerStatus.is_valid())
  {
    for (uint8_t i = 0; i < physical_constants::batterySeriesCount; ++i)
    {
      const auto cellVoltage = balancerStatus.batteryVoltages_mV[i];
      // check voltage validity, than check if if in bounds
      if (is_cell_voltage_valid(cellVoltage) and cellVoltage >= physical_constants::minSingularBatteryVoltage_mV and
          cellVoltage < minCellVoltage)
        minCellVoltage = cellVoltage;
    }
  }

  // no min cell voltage, maybe balancer is disconnected
  if (minCellVoltage == physical_constants::maxSingularBatteryVoltage_mV)
  {
    return get_battery_level();
  }
  else
  {
    // get the result of the total battery life, map it to the safe battery level
    // indicated by user
    return get_level_safe(minCellVoltage, 1);
  }
}

uint16_t get_battery_maximum_cell_level()
{
  uint16_t maxCellVoltage = physical_constants::minSingularBatteryVoltage_mV;
  const auto& balancerStatus = bsp::balancer::get_status();
  if (balancerStatus.is_valid())
  {
    for (uint8_t i = 0; i < physical_constants::batterySeriesCount; ++i)
    {
      const auto cellVoltage = balancerStatus.batteryVoltages_mV[i];
      // check voltage validity, than keep greatest voltage
      if (is_cell_voltage_valid(cellVoltage) and cellVoltage > maxCellVoltage)
        maxCellVoltage = cellVoltage;
    }
  }

  // no min cell voltage, maybe balancer is disconected
  if (maxCellVoltage == physical_constants::minSingularBatteryVoltage_mV)
  {
    return get_battery_level();
  }
  else
  {
    // get the result of the total battery life, map it to the safe battery level
    // indicated by user
    return get_level_safe(maxCellVoltage, 1);
  }
}

} // namespace battery
} // namespace component
} // namespace lampda
