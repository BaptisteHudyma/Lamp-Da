#include "battery.h"

#include "src/system/logic/alerts.h"

#include "src/system/utils/constants.h"

#include "src/system/power/charger.h"
#include "src/system/power/balancer.h"

namespace battery {

static uint16_t s_batteryVoltage_mV = 0;

void check_individual_batteries(const balancer::Status& balancerStatus)
{
#if 0 // TODO finish the cell check below
  // in bounds with some margin
  static constexpr uint16_t minBatteryValue = batteryMinVoltage_mV * 0.95;
  static constexpr uint16_t maxBatteryValue =
          batteryMaxVoltage_mV * 1.01;

  uint16_t packVoltage = 0;
  // check individual battery voltages
  for (uint8_t i = 0; i < batteryCount; ++i)
  {
    const uint16_t batteryVoltage = balancerStatus.batteryVoltages_mV[i];
    if (batteryVoltage == 0)
    {
      // disconected battery
      // or wrong read
      // This is not good, but ok the lamp can survive without.
      // It will unbalance after a time, and endup not working after a lot of charge/discharge cycles
    }
    else if (not is_cell_voltage_valid(batteryVoltage))
    {
      // TODO: alert battery voltage is incoherent
    }
    packVoltage += batteryVoltage;
  }

  // incoherent with the pack voltage. Disconnected battery ?
  if (packVoltage < balancerStatus.stackVoltage_mV - 50)
  {
    // TODO: alert disconected battery
  }
#endif
}

/**
 * \brief check the battery pack voltage, and individual cell voltages
 * This should be the main used function.
 * \return false in case of a problem
 */
bool check_balancer_battery_voltage()
{
  const auto& balancerStatus = balancer::get_status();
  if (balancerStatus.is_valid())
  {
    s_batteryVoltage_mV = balancerStatus.stackVoltage_mV;

    // check each battery voltage for validity
    check_individual_batteries(balancerStatus);
    return true;
  }
  return false;
}

/**
 * \brief check the battery voltage from charging component
 */
bool check_charger_battery_voltage()
{
  const auto& chargerStates = charger::get_state();
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
  // WILL UPDATE s_batteryVoltage_mV VALUE if they return true
  if (not check_balancer_battery_voltage() and not check_charger_battery_voltage())
  {
    // else: not ready yet ? error, return max voltage for now
    // TODO: after a set time, return an error, the system should not be used without batteries
    return 0;
  }

  // absolute minimum/maximum battery pack voltage
  static constexpr uint16_t minBatteryVoltage_mV = minSingularBatteryVoltage_mV * batteryCount;
  static constexpr uint16_t maxBatteryVoltage_mV = maxSingularBatteryVoltage_mV * batteryCount;
  // check battery pack validity (in bounds with some margin)
  if (s_batteryVoltage_mV < minBatteryVoltage_mV or s_batteryVoltage_mV > maxBatteryVoltage_mV)
  {
    alerts::manager.raise(alerts::Type::BATTERY_READINGS_INCOHERENT);
    // return a default value
    return 0;
  }
  // return the true battery voltage
  return s_batteryVoltage_mV;
}

bool is_battery_usable_as_power_source()
{
  // TODO implement: return false if
  // - stack reading incoherent
  // - first battery of the stack is disconnected
  // - a battery voltage is below the safety voltage
  return alerts::manager.can_use_output_power();
}

bool can_battery_be_charged()
{
  // TODO: implement: return false if
  // - first or last battery of the stack are disconected
  // - ?
  return alerts::manager.can_charge_battery();
}

uint16_t get_battery_minimum_cell_level()
{
  uint16_t minCellVoltage = maxSingularBatteryVoltage_mV;
  const auto& balancerStatus = balancer::get_status();
  if (balancerStatus.is_valid())
  {
    for (uint8_t i = 0; i < batteryCount; ++i)
    {
      const auto cellVoltage = balancerStatus.batteryVoltages_mV[i];
      // TODO: check voltage validity
      if (cellVoltage >= minSingularBatteryVoltage_mV and cellVoltage < minCellVoltage)
        minCellVoltage = cellVoltage;
    }
  }

  // no min cell voltage, maybe balancer is disconnected
  if (minCellVoltage == maxSingularBatteryVoltage_mV)
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
  uint16_t maxCellVoltage = minSingularBatteryVoltage_mV;
  const auto& balancerStatus = balancer::get_status();
  if (balancerStatus.is_valid())
  {
    for (uint8_t i = 0; i < batteryCount; ++i)
    {
      const auto cellVoltage = balancerStatus.batteryVoltages_mV[i];
      // TODO: check voltage validity
      if (cellVoltage > maxCellVoltage)
        maxCellVoltage = cellVoltage;
    }
  }

  // no min cell voltage, maybe balancer is disconected
  if (maxCellVoltage == minSingularBatteryVoltage_mV)
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
