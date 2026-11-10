/*! \file charge_policy.h
    \brief Rules deciding when the battery charge must stop.

    Pure logic on a measurement snapshot, so that the rules can be unit tested without the charger hardware.
*/

#ifndef COMPONENT_CHARGE_POLICY_H
#define COMPONENT_CHARGE_POLICY_H

#include <cstdint>

#include "src/system/utils/constants.h"

namespace lampda {
namespace component {
namespace charger {
/// Charge stop rules, from a snapshot of the battery measurements
namespace policy {

/**
 * The charging IC regulates the pack voltage on its own reference (+-0.5%, ~60mV at 12.3V) and does not see the cells:
 * it can settle above the target. The charge is stopped from the balancer measurements, that are more precise.
 */

/// Battery level (percent * 100) from which the battery is considered full
static constexpr uint16_t fullLevel = 9900;
/// Once full, the charge restarts below this battery level (percent * 100)
static constexpr uint16_t rechargeLevel = 9500;
/// The end of charge condition must stay true for this time before the charge is considered done
static constexpr uint32_t endOfChargeDeglitch_ms = 5000;

/// Accepted cell voltage above the safe maximum (cell imbalance, measure noise)
static constexpr uint16_t cellOverVoltageMargin_mV = 10;
/// A cell over this voltage stops the charge immediately
static constexpr uint16_t cellOverVoltage_mV = physical_constants::maxSafeLiionVoltage_mV + cellOverVoltageMargin_mV;
/// A pack over this voltage stops the charge immediately
static constexpr uint16_t packOverVoltage_mV = physical_constants::batteryMaxVoltageSafe_mV +
                                               cellOverVoltageMargin_mV * physical_constants::batterySeriesCount;
/// After an over voltage, the charge can restart when all cells are below this voltage
static constexpr uint16_t cellResumeVoltage_mV = physical_constants::maxSafeLiionVoltage_mV - 20;
/// After an over voltage, the charge stays stopped at least this long (lets the balancer work)
static constexpr uint32_t overVoltagePause_ms = 20000;

/// Snapshot of the measurements used by the charge stop rules
struct Inputs
{
  /// True if the balancer measurements below are valid
  bool isBalancerValid = false;
  /// Highest cell voltage, from the balancer
  uint16_t maxCell_mV = 0;
  /// Pack voltage, from the balancer
  uint16_t pack_mV = 0;
  /// Battery level, in percent * 100
  uint16_t level = 0;
  /// Measured charge current
  uint16_t chargeCurrent_mA = 0;
  /// True if the charger is charging
  bool isCharging = false;
};

/// True if a cell or the pack is over the charge voltage limit
inline bool is_over_voltage(const Inputs& in)
{
  return in.isBalancerValid and (in.maxCell_mV > cellOverVoltage_mV or in.pack_mV > packOverVoltage_mV);
}

/// True if the battery reached the end of charge: pack at the target voltage, or charge current tapered off near full
inline bool is_end_of_charge(const Inputs& in)
{
  if (not in.isCharging)
    return false;
  // pack voltage reached the target
  if (in.isBalancerValid and in.pack_mV >= physical_constants::batteryMaxVoltageSafe_mV)
    return true;
  // constant voltage phase, current tapered off.
  // The level condition prevents a weak power source (low charge current) to be mistaken for an end of charge
  return in.level >= fullLevel and in.chargeCurrent_mA <= physical_constants::batteryEndOfChargeCurrent_mA;
}

/**
 * \brief Track the charge stop state (battery full, over voltage)
 */
class ChargeLimiter
{
public:
  /**
   * \brief Update the state with new measurements
   * \return true if the charge is allowed
   */
  bool update(const uint32_t time_ms, const Inputs& in)
  {
    // safety: over voltage stops the charge, whatever the state
    if (is_over_voltage(in))
    {
      _isOverVoltage = true;
      _overVoltageTime_ms = time_ms;
      _isEndOfChargeDeglitching = false;
      // already full: end the charge
      if (in.level >= fullLevel)
        _isFull = true;
      return false;
    }

    // stay stopped until the cells are back under the resume voltage
    if (_isOverVoltage)
    {
      if (time_ms - _overVoltageTime_ms < overVoltagePause_ms or not in.isBalancerValid or
          in.maxCell_mV >= cellResumeVoltage_mV)
        return false;
      _isOverVoltage = false;
    }

    // battery full: wait for the level to drop
    if (_isFull)
    {
      if (in.level >= rechargeLevel)
        return false;
      _isFull = false;
    }

    if (is_end_of_charge(in))
    {
      if (not _isEndOfChargeDeglitching)
      {
        _isEndOfChargeDeglitching = true;
        _endOfChargeTime_ms = time_ms;
      }
      else if (time_ms - _endOfChargeTime_ms > endOfChargeDeglitch_ms)
      {
        _isEndOfChargeDeglitching = false;
        _isFull = true;
        return false;
      }
    }
    else
    {
      _isEndOfChargeDeglitching = false;
    }
    return true;
  }

  /// Forget the battery full state (power source change). The over voltage state is kept, for safety
  void clear_full()
  {
    _isFull = false;
    _isEndOfChargeDeglitching = false;
  }

  /// True if the battery charge is done
  bool is_full() const { return _isFull; }
  /// True if the charge is stopped by an over voltage
  bool is_over_voltage_stopped() const { return _isOverVoltage; }

private:
  bool _isFull = false;
  bool _isOverVoltage = false;
  uint32_t _overVoltageTime_ms = 0;
  bool _isEndOfChargeDeglitching = false;
  uint32_t _endOfChargeTime_ms = 0;
};

} // namespace policy
} // namespace charger
} // namespace component
} // namespace lampda

#endif
