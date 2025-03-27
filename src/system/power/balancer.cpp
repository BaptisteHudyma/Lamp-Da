#include "balancer.h"

#include "src/system/platform/time.h"

#include "drivers/BQ76905.h"

namespace balancer {

// threshold at which point a battery is considered unbalanced with another
constexpr uint16_t unbalancedMv = 10;

bool Status::is_valid() const
{
  if (lastMeasurmentUpdate == 0 or time_ms() - lastMeasurmentUpdate >= 1000)
  {
    return false;
  }
  return true;
}

bool Status::is_balanced() const
{
  // should never be called
  if (!is_valid())
    return true;

  const uint16_t typicalVoltage = batteryVoltages_mV[0];
  for (const uint16_t batVolt: batteryVoltages_mV)
  {
    // battery voltage
    if (batVolt < typicalVoltage - unbalancedMv or batVolt > typicalVoltage + unbalancedMv)
    {
      return false;
    }
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

  if constexpr (batteryCount == 2)
  {
    switch (index)
    {
      case 0:
        return balancerRegisters.Cell1Voltage.get();
      case 1:
        return balancerRegisters.Cell5Voltage.get();
    }
  }
  else if constexpr (batteryCount == 3)
  {
    switch (index)
    {
      case 0:
        return balancerRegisters.Cell1Voltage.get();
      case 1:
        return balancerRegisters.Cell2Voltage.get();
      case 2:
        return balancerRegisters.Cell5Voltage.get();
    }
  }
  else if constexpr (batteryCount == 4)
  {
    switch (index)
    {
      case 0:
        return balancerRegisters.Cell1Voltage.get();
      case 1:
        return balancerRegisters.Cell2Voltage.get();
      case 2:
        return balancerRegisters.Cell4Voltage.get();
      case 3:
        return balancerRegisters.Cell5Voltage.get();
    }
  }
  else if constexpr (batteryCount == 5)
  {
    switch (index)
    {
      case 0:
        return balancerRegisters.Cell1Voltage.get();
      case 1:
        return balancerRegisters.Cell2Voltage.get();
      case 2:
        return balancerRegisters.Cell3Voltage.get();
      case 3:
        return balancerRegisters.Cell4Voltage.get();
      case 4:
        return balancerRegisters.Cell5Voltage.get();
    }
  }

  // never reachable
  return 0;
}

uint16_t get_device_number() { return balancerRegisters.FirmwareVersion.get_device_number(); }

void balance_batteries()
{
  // TODO
}

void disable_battery_balancing()
{
  // TODO
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
  balancerRegisters.ExitDeepSleep.send();

  // check device id
  if (balancerRegisters.DeviceNumber.get() != bq76905::DEVICE_NUMBER)
  {
    // error: those constants do not indicate a BQ76905
    // charger
    return false;
  }

  // enable full scan
  balancer.readRegEx(balancerRegisters.AlarmEnable);
  balancerRegisters.AlarmEnable.set_ADSCAN(1);
  balancerRegisters.AlarmEnable.set_FULLSCAN(1);
  balancer.writeRegEx(balancerRegisters.AlarmEnable);

  // set number of cells
  balancerRegisters.ConfigurateVcell.set(batteryCount);

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
    balancer.readRegEx(balancerRegisters.AlarmStatus);
    // accept value
    if (balancerRegisters.AlarmStatus.FULLSCAN() == 0)
      return;
    isInitFirstFullScanDone = true;
  }

  // refresh
  const uint32_t time = time_ms();
  if (_status.lastMeasurmentUpdate == 0 or time - _status.lastMeasurmentUpdate > 800)
  {
    _status.stackVoltage_mV = balancerRegisters.StackVoltage.get();
    _status.temperature_degrees = balancerRegisters.IntTemperatureVoltage.get();

    // set battery voltages
    for (uint8_t i = 0; i < batteryCount; ++i)
    {
      _status.batteryVoltages_mV[i] = get_battery_voltage_mv(i);
    }

    _status.lastMeasurmentUpdate = time;
  }

  // if the balancing process is enabled, balance batteries
  if (isBalancingEnabled)
  {
    static uint32_t lastBalancingUpdateTime = 0;
    if (time - lastBalancingUpdateTime > 1000)
    {
      balance_batteries();
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
  balancerRegisters.DeepSleep.send();
  delay(10);
  balancerRegisters.DeepSleep.send();
  delay(10);
  balancerRegisters.DeepSleep.send();
}

} // namespace balancer