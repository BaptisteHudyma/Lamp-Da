#ifndef BATTERY_H
#define BATTERY_H

#include <cstdint>

#include "src/system/utils/curves.h"
#include "src/system/utils/utils.h"

namespace battery {

// get the battery voltage, untreated
extern uint16_t get_raw_battery_voltage_mv();

/**
 * \brief return true if the battery pack can be used as a energy source
 * This status is updated after calling \ref get_raw_battery_voltage_mv
 */
extern bool is_battery_usable_as_power_source();
/**
 * \brief Return true if this battery can be charged
 * Check only for validity, not voltage.
 * If this is false, starting a charge process can break the system
 * This status is updated after calling \ref get_raw_battery_voltage_mv
 */
extern bool can_battery_be_charged();

/**
 * \brief convert a single liion battery voltage to a percent level model
 * \param[in] liionLevel_mv The voltage of the whole battery
 * \param[in] batteryCountSerie How many liion cells in serie
 * \return The battery percent, x100
 */
inline uint16_t liion_mv_to_battery_percent(const uint16_t liionLevel_mv, const uint8_t batteryCountSerie)
{
  using curve_t = curves::LinearCurve<uint16_t, uint16_t>;
  static curve_t liionVoltagePercentToRealPercent({// low end of the curve, sharp drop
                                                   curve_t::point_t {3000, 0},
                                                   curve_t::point_t {3210, 500},
                                                   curve_t::point_t {3350, 1000},
                                                   curve_t::point_t {3430, 1500},
                                                   curve_t::point_t {3470, 2000},
                                                   // linear approximation works well enough in range 20-90%
                                                   curve_t::point_t {4080, 9000},
                                                   curve_t::point_t {4110, 9500},
                                                   curve_t::point_t {4180, 10000}});

  // sample the curve
  return liionVoltagePercentToRealPercent.sample(liionLevel_mv / batteryCountSerie);
}

/**
 * \brief transform a battery voltage measurment to a percent * 100 estimate
 * \param[in] batteryLevel_mV the measured battery voltage
 * \return the battery level, in percent * 100
 */
inline uint16_t get_level_percent(const uint16_t batteryVoltage_mV, const uint8_t cellCount = batteryCount)
{
  return liion_mv_to_battery_percent(batteryVoltage_mV, cellCount);
}

/**
 * \brief returns the battery level, mapped to the desired safe battery level
 */
inline uint16_t get_level_safe(const uint16_t battery_mv, const uint8_t cellCount = batteryCount)
{
  // save the init values
  static const uint16_t minSafeLevel_percent = get_level_percent(batteryMinVoltageSafe_mV);
  static const uint16_t maxSafeLevel_percent = get_level_percent(batteryMaxVoltageSafe_mV);

  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return lmpd_constrain<uint16_t>(
          lmpd_map<uint16_t>(
                  get_level_percent(battery_mv, cellCount), minSafeLevel_percent, maxSafeLevel_percent, 0, 10000),
          0,
          10000);
}

// returns the battery level, corresponding to user safe choice (0-10000)
inline uint16_t get_battery_level()
{
  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return get_level_safe(get_raw_battery_voltage_mv());
}

/**
 * \brief Return the level of the cell of the stack with the minimum battery
 */
uint16_t get_battery_minimum_cell_level();

/**
 * \brief Return the level of the cell of the stack with the maximum battery
 */
uint16_t get_battery_maximum_cell_level();

} // namespace battery

#endif
