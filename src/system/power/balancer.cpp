#include "balancer.h"

#include "src/system/platform/time.h"
#include "src/system/utils/print.h"

#include "drivers/BQ76905.h"
#include <cstdint>

namespace balancer {

// threshold at which point a battery is considered unbalanced with another
constexpr uint16_t unbalancedMv = 3;

static_assert(batteryCount == 3, "The balancing code only supports 3 cells");

bool Status::is_valid() const
{
  if (lastMeasurmentUpdate == 0 or time_ms() - lastMeasurmentUpdate >= 1000)
  {
    return false;
  }
  return true;
}

Status _status;

bq76905::BQ76905 balancer;
bq76905::BQ76905::Regt balancerRegisters;

bool isBalancingEnabled = false;

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
  }

  // never reachable
  return 0;
}

// 3 batteries version
void set_balancing(uint8_t battery1, uint8_t battery2, uint8_t battery3)
{
  balancerRegisters.cbActiveCells.set_CBCELLS_0(battery1 & 1);
  balancerRegisters.cbActiveCells.set_CBCELLS_1(battery2 & 1);
  balancerRegisters.cbActiveCells.set_CBCELLS_2(battery3 & 1);
  balancerRegisters.cbActiveCells.set_CBCELLS_3(0);
  balancerRegisters.cbActiveCells.set_CBCELLS_4(0);
  balancerRegisters.cbActiveCells.write();
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
    if (status.batteryVoltages_mV[i] >= minSingularBatteryVoltage_mV and
        status.batteryVoltages_mV[i] < batteryVoltageMin)
      batteryVoltageMin = status.batteryVoltages_mV[i];
  }

  balancerRegisters.cbActiveCells.read_reg();

  bool isBalancing[batteryCount];
  isBalancing[0] = balancerRegisters.cbActiveCells.CBCELLS_0();
  isBalancing[1] = balancerRegisters.cbActiveCells.CBCELLS_1();
  isBalancing[2] = balancerRegisters.cbActiveCells.CBCELLS_2();

  // all cells too far above should be throttled down
  bool hasChanged = false;
  uint8_t set[batteryCount];
  for (uint8_t i = 0; i < batteryCount; i++)
  {
    // set the cell to balance if too far from the mean
    if (isBalancing[i] == 0)
      set[i] = (status.batteryVoltages_mV[i] >= (batteryVoltageMin + unbalancedMv)) ? 1 : 0;
    // is already balancing, latch until we reach the same (sameish) voltage
    else
      set[i] = (status.batteryVoltages_mV[i] > batteryVoltageMin) ? 1 : 0;

    if (isBalancing[i] != set[i])
      hasChanged = true;
  }
  // balance
  if (hasChanged)
    set_balancing(set[0], set[1], set[2]);
}

void disable_battery_balancing()
{
  // turn off all balancing
  set_balancing(0, 0, 0);
}

Status get_status() { return _status; }

bool isInit = false;
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
  balancer.writeRegEx(balancerRegisters.alarmEnable);

  // deactivate external temperature sensing and pullup on TS
  balancerRegisters.settingConfiguration_DA.read_reg();
  balancerRegisters.settingConfiguration_DA.set_TSMODE(1);
  balancerRegisters.settingConfiguration_DA.write();

  // offset to prevent balancing lockup
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

bool isInitFirstFullScanDone = false;
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

  // refresh
  const uint32_t time = time_ms();
  if (_status.lastMeasurmentUpdate == 0 or time - _status.lastMeasurmentUpdate > 800)
  {
    _status.stackVoltage_mV = balancerRegisters.stackVoltage.get();
    _status.temperature_degrees = balancerRegisters.intTemperatureVoltage.get();

    // set battery voltages
    for (uint8_t i = 0; i < batteryCount; ++i)
    {
      _status.batteryVoltages_mV[i] = get_battery_voltage_mv(i);
    }

    // set is balancing status
    balancer.readRegEx(balancerRegisters.alarmStatus);

    balancerRegisters.cbActiveCells.read_reg();
    _status.isBalancing[0] = (balancerRegisters.cbActiveCells.CBCELLS_0() != 0x00);
    _status.isBalancing[1] = (balancerRegisters.cbActiveCells.CBCELLS_1() != 0x00);
    _status.isBalancing[2] = (balancerRegisters.cbActiveCells.CBCELLS_2() != 0x00);

    _status.lastMeasurmentUpdate = time;
  }

  // if the balancing process is enabled, balance batteries
  if (isBalancingEnabled)
  {
    static uint32_t lastBalancingUpdateTime = 0;
    if (time - lastBalancingUpdateTime > 1000)
    {
      balance_batteries();
      lastBalancingUpdateTime = time;
    }
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