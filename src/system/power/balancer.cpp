#include "balancer.h"

#include "src/system/logic/alerts.h"

#include "src/system/platform/time.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/print.h"

#include "src/system/utils/time_utils.h"

#include <cstdint>

// use depend of component
#include "depends/BQ76905/BQ76905.h"

namespace balancer {

// threshold at which point a battery is considered unbalanced with another
constexpr uint16_t unbalancedMv = 3;

//
static_assert(HARDWARE_VERSION_MAJOR == 1 and batteryCount == 3,
              "The 1.X hardware only supports 3 cells for balancing");

bool Status::is_valid() const
{
  if (lastMeasurmentUpdate == 0 or time_ms() - lastMeasurmentUpdate >= 1000)
  {
    return false;
  }
  return true;
}

Status _status;

using balancer_t = bq76905::BQ76905;
balancer_t balancer;
balancer_t::Regt balancerRegisters;

bool isBalancingEnabled = false;

bool isInit = false;
bool isInitFirstFullScanDone = false;

uint16_t get_battery_voltage_mv(const uint8_t index)
{
  static_assert(batteryCount >= 2 and batteryCount <= 5, "balancer can only handle 2 to 5 batteries");
  if (index >= batteryCount)
    return 0;

  switch (index)
  {
    case 0:
      return balancerRegisters.cell1Voltage.get();
    case 1:
      return balancerRegisters.cell2Voltage.get();
    case 2:
      return balancerRegisters.cell3Voltage.get();
    case 3:
      return balancerRegisters.cell4Voltage.get();
    case 4:
      return balancerRegisters.cell5Voltage.get();
    default:
      break;
  }

  // never reachable
  return 0;
}

/**
 * \brief Compute the maximum number of cells that should be balanced at the same time
 * prevent the breakage of the component at high temperatures
 */
uint8_t compute_cell_balancing_max()
{
  if (_status.is_valid())
  {
    for (uint8_t i = 0; i < 5; i++)
    {
      // augment the temperature threshold, until 30 + 4 * 10 => 70 degrees
      if (_status.temperature_degrees < 30 + i * 10)
      {
        // transform the index into a cell count (5 to 1)
        return 5 - i;
      }
    }
  }
  return 0;
}

// user should write the register after calling this function
// balancerRegisters.cbActiveCells.write();
void set_balancing(uint8_t cellIndex, bool shouldBalance)
{
  static_assert(batteryCount >= 2 and batteryCount <= 5, "balancer can only handle 2 to 5 batteries");
  if (cellIndex >= batteryCount)
    return;

  switch (cellIndex)
  {
    case 0:
      balancerRegisters.cbActiveCells.set_CBCELLS_0(shouldBalance ? 1 : 0);
      break;
    case 1:
      balancerRegisters.cbActiveCells.set_CBCELLS_1(shouldBalance ? 1 : 0);
      break;
    case 2:
      balancerRegisters.cbActiveCells.set_CBCELLS_2(shouldBalance ? 1 : 0);
      break;
    case 3:
      balancerRegisters.cbActiveCells.set_CBCELLS_3(shouldBalance ? 1 : 0);
      break;
    case 4:
      balancerRegisters.cbActiveCells.set_CBCELLS_4(shouldBalance ? 1 : 0);
      break;
    default:
      break;
  }

  // never reachable
  return;
}

// user should read the register before calling this function
// balancerRegisters.cbActiveCells.read_reg();
bool is_balancing(uint8_t cellIndex)
{
  static_assert(batteryCount >= 2 and batteryCount <= 5, "balancer can only handle 2 to 5 batteries");
  if (cellIndex >= batteryCount)
    return false;

  switch (cellIndex)
  {
    case 0:
      return balancerRegisters.cbActiveCells.CBCELLS_0() != 0;
    case 1:
      return balancerRegisters.cbActiveCells.CBCELLS_1() != 0;
    case 2:
      return balancerRegisters.cbActiveCells.CBCELLS_2() != 0;
    case 3:
      return balancerRegisters.cbActiveCells.CBCELLS_3() != 0;
    case 4:
      return balancerRegisters.cbActiveCells.CBCELLS_4() != 0;
    default:
      break;
  }

  // never reachable
  return false;
}

void balance_batteries()
{
  const auto& status = _status;
  if (not _status.is_valid())
    return;

  // min cell voltage
  uint16_t batteryVoltageMin = maxSingularBatteryVoltage_mV;
  for (uint8_t i = 0; i < batteryCount; i++)
  {
    // do not use invalid voltage as balancing reference
    if (is_cell_voltage_valid(status.batteryVoltages_mV[i]) and status.batteryVoltages_mV[i] < batteryVoltageMin)
      batteryVoltageMin = status.batteryVoltages_mV[i];
  }

  uint8_t maxCellsToBalance = compute_cell_balancing_max();

  // read active battery register
  balancerRegisters.cbActiveCells.read_reg();

  // all cells too far above should be throttled down
  bool hasChanged = false;
  for (uint8_t i = 0; i < batteryCount; i++)
  {
    const bool isBalancing = is_balancing(i);

    bool shouldBalance = false;

    // no cells to balance left
    if (maxCellsToBalance == 0)
    {
      shouldBalance = false;
    }
    // battery is too high, and should be emptied a litte bit
    else if (status.batteryVoltages_mV[i] >= maxLiionVoltage_mV)
    {
      shouldBalance = true;
    }
    else
    {
      // set the cell to balance if too far from the mean
      if (not isBalancing)
        shouldBalance = status.batteryVoltages_mV[i] >= (batteryVoltageMin + unbalancedMv);
      // is already balancing, latch until we reach the same (sameish) voltage
      else
        shouldBalance = status.batteryVoltages_mV[i] > batteryVoltageMin;
    }

    // limit the cell to balance in parallel
    if (shouldBalance)
      maxCellsToBalance--;

    if (isBalancing != shouldBalance)
    {
      set_balancing(i, shouldBalance);
      hasChanged = true;
    }
  }
  // only write the command if anything changed
  if (hasChanged)
  {
    balancerRegisters.cbActiveCells.write();
  }
}

void disable_battery_balancing()
{
  // turn off all balancing
  for (uint8_t i = 0; i < batteryCount; i++)
  {
    set_balancing(i, false);
  }
  balancerRegisters.cbActiveCells.write();
}

Status get_status() { return _status; }

bool init()
{
  if (i2c_check_existence(bq76905::i2cObjectIndex, bq76905::BQ76905::BQ76905addr) != 0)
  {
    // error: device not detected
    // charger
    return false;
  }

  // wake up the system
  balancerRegisters.exitDeepSleep.send();

  // check device id
  if (balancerRegisters.deviceNumber.get() != bq76905::DEVICE_NUMBER)
  {
    // error: those constants do not indicate a BQ76905
    // charger
    return false;
  }

  // reset registers
  balancerRegisters.reset.send();
  delay_ms(5);

  // got to config mode
  balancerRegisters.setCfgUpdate.send();

  // enable full scan
  balancer.readRegEx(balancerRegisters.alarmEnable);
  balancerRegisters.alarmEnable.set_ADSCAN(1);   // signal value scan
  balancerRegisters.alarmEnable.set_FULLSCAN(1); // signal value scan
  balancerRegisters.alarmEnable.set_CB(1);       // signal cell balancing
  balancer_t::writeRegEx(balancerRegisters.alarmEnable);

  // deactivate external temperature sensing and pullup on TS
  balancerRegisters.settingConfiguration_DA.read_reg();
  balancerRegisters.settingConfiguration_DA.set_TSMODE(1);
  balancerRegisters.settingConfiguration_DA.write();

  // offset to prevent balancing lockup
  // TODO more reliable offset
  balancerRegisters.tsOffset.set(-10);

  // set number of cells
  balancerRegisters.configurateVcell.read_reg();
  balancerRegisters.configurateVcell.set_VCELL(batteryCount);
  balancerRegisters.configurateVcell.write();

  // exist config mode after startup
  balancerRegisters.exitCfgUpdate.send();

  isInit = true;
  return true;
}

void loop()
{
  if (not isInit)
    return;

  if (not isInitFirstFullScanDone)
  {
    balancer.readRegEx(balancerRegisters.alarmStatus);
    // wait for first measurment
    if (balancerRegisters.alarmStatus.FULLSCAN() == 0)
      return;
    isInitFirstFullScanDone = true;
  }

  static bool isBalancingAllowed = true;

  // refresh
  EVERY_N_MILLIS(500)
  {
    isBalancingAllowed = true;
    _status.stackVoltage_mV = balancerRegisters.stackVoltage.get();
    _status.temperature_degrees = balancerRegisters.intTemperatureVoltage.get();

    balancerRegisters.cbActiveCells.read_reg();
    // set battery voltages, and balancing status
    for (uint8_t i = 0; i < batteryCount; ++i)
    {
      _status.batteryVoltages_mV[i] = get_battery_voltage_mv(i);
      _status.isBalancing[i] = is_balancing(i);

      if (not is_cell_voltage_valid(_status.batteryVoltages_mV[i]))
      {
        isBalancingAllowed = false;
      }
    }

    if (not isBalancingAllowed)
    {
      alerts::manager.raise(alerts::Type::BATTERY_READINGS_INCOHERENT);
      disable_battery_balancing();
    }

    // update measurments
    _status.lastMeasurmentUpdate = time_ms();
  }

  // if the balancing process is enabled, balance batteries
  if (isBalancingEnabled && isBalancingAllowed)
  {
    EVERY_N_MILLIS(1000) { balance_batteries(); }
  }
}

void enable_balancing(bool enable)
{
  if (enable)
  {
    isBalancingEnabled = true;
  }
  else
  {
    if (isBalancingEnabled)
      disable_battery_balancing();

    isBalancingEnabled = false;
  }
}

void go_to_sleep()
{
  disable_battery_balancing();

  // send 3 times, with delay between each
  balancerRegisters.deepSleep.send();
  delay_ms(10);
  balancerRegisters.deepSleep.send();
}

} // namespace balancer
