#ifndef BATTERY_H
#define BATTERY_H

#include <cstdint>

#include "src/system/utils/curves.h"
#include "src/system/utils/utils.h"

namespace battery {

// return a number between 0 and 10000 (% * 100)
// it corresponds to the real battery level
extern uint16_t get_raw_battery_level();

// returns the battery level, corresponding to user safe choice (0-10000)
extern uint16_t get_battery_level();

extern void raise_battery_alert();

// convert a liion battery level to a linear model
inline uint16_t liion_level_to_battery_percent(const uint16_t liionLevelPercent)
{
  using curve_t = curves::LinearCurve<uint16_t, uint16_t>;
  static curve_t liionVoltagePercentToRealPercent({curve_t::point_t {0, 0},
                                                   curve_t::point_t {4000, 1200},
                                                   curve_t::point_t {9000, 9500},
                                                   curve_t::point_t {10000, 10000}});

  // sample the curve
  return liionVoltagePercentToRealPercent.sample(liionLevelPercent);
}

/**
 * \brief transform a battery voltage measurment to a percent * 100 estimate
 * \param[in] batteryLevel_mV the measured battery level
 * \return the battery level, in percent * 100
 */
inline uint16_t get_level_percent(const uint16_t batteryLevel_mV)
{
  return liion_level_to_battery_percent(lmpd_constrain(
          lmpd_map<uint16_t, uint16_t>(batteryLevel_mV, batteryMinVoltage_mV, batteryMaxVoltage_mV, 0, 10000), 0, 10000));
}

/**
 * \brief Return the max safe battery level, in percent * 100
 */
inline uint16_t get_max_safe_level() { return get_level_percent(batteryMaxVoltageSafe_mV); }

/**
 * \brief Return the min safe battery level, in percent * 100
 */
inline uint16_t get_min_safe_level() { return get_level_percent(batteryMinVoltageSafe_mV); }

/**
 * \brief returns the battery level, mapped to the desired safe battery level
 */
inline uint16_t get_level(const uint16_t batteryLevel)
{
  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return lmpd_constrain(
          lmpd_map<uint16_t, uint16_t>(batteryLevel, get_min_safe_level(), get_max_safe_level(), 0, 10000), 0, 10000);
}

} // namespace battery

#endif