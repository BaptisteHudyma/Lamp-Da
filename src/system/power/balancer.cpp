#include "balancer.h"

#include "src/system/platform/time.h"
#include "src/system/utils/print.h"

#include "drivers/BQ76905.h"
#include <cstdint>

namespace balancer {

// threshold at which point a battery is considered unbalanced with another
constexpr uint16_t unbalancedMv = 5;

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
  balancerRegisters.cbActiveCells.set_balancing(battery1, battery2, 0, 0, battery3);
  /*
    lampda_print("%d %d %d", 0x0A, balancerRegisters.cbActiveCells.get(), balancerRegisters.tsMeasurmentVoltage.get());

    balancer.readRegEx(balancerRegisters.safetyAlertA);
    balancer.readRegEx(balancerRegisters.safetyAlertB);
    balancer.readRegEx(balancerRegisters.alarmEnable);
    balancer.readRegEx(balancerRegisters.batteryStatus);
    balancer.readRegEx(balancerRegisters.alarmStatus);
    lampda_print("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                 balancerRegisters.safetyAlertA.COV_ALERT(),
                 balancerRegisters.safetyAlertA.CUV_ALERT(),
                 balancerRegisters.safetyAlertA.OCC_ALERT(),
                 balancerRegisters.safetyAlertA.OCD1_ALERT(),
                 balancerRegisters.safetyAlertA.OCD2_ALERT(),
                 balancerRegisters.safetyAlertA.SCD_ALERT(),
                 balancerRegisters.safetyAlertA.COV_FAULT(),
                 balancerRegisters.safetyAlertA.OCD1_ALERT(),
                 balancerRegisters.safetyAlertA.OCD1_FAULT(),
                 balancerRegisters.safetyAlertA.OCD2_ALERT(),
                 balancerRegisters.safetyAlertA.OCD2_FAULT(),
                 balancerRegisters.safetyAlertA.SCD_ALERT(),
                 balancerRegisters.safetyAlertA.SCD_FAULT(),
                 balancerRegisters.safetyAlertA.CURLATCH(),
                 balancerRegisters.safetyAlertA.REGOUT());
    lampda_print("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                 balancerRegisters.safetyAlertB.OTD_ALERT(),
                 balancerRegisters.safetyAlertB.OTC_ALERT(),
                 balancerRegisters.safetyAlertB.UTD_ALERT(),
                 balancerRegisters.safetyAlertB.UTC_ALERT(),
                 balancerRegisters.safetyAlertB.OTINT_ALERT(),
                 balancerRegisters.safetyAlertB.HWD_ALERT(),
                 balancerRegisters.safetyAlertB.VREF_ALERT(),
                 balancerRegisters.safetyAlertB.VSS_ALERT(),
                 balancerRegisters.safetyAlertB.OTD_FAULT(),
                 balancerRegisters.safetyAlertB.OTC_FAULT(),
                 balancerRegisters.safetyAlertB.UTD_FAULT(),
                 balancerRegisters.safetyAlertB.UTC_FAULT(),
                 balancerRegisters.safetyAlertB.OTINT_FAULT(),
                 balancerRegisters.safetyAlertB.HWD_FAULT(),
                 balancerRegisters.safetyAlertB.VREF_FAULT(),
                 balancerRegisters.safetyAlertB.VSS_FAULT());
    lampda_print("%d %d %d %d %d %d %d %d %d %d %d %d %d",
                 balancerRegisters.batteryStatus.POR(),
                 balancerRegisters.batteryStatus.SLEEP_EN(),
                 balancerRegisters.batteryStatus.CFGUPDATE(),
                 balancerRegisters.batteryStatus.ALERTPIN(),
                 balancerRegisters.batteryStatus.CHG(),
                 balancerRegisters.batteryStatus.DSG(),
                 balancerRegisters.batteryStatus.CHGDETFLAG(),
                 balancerRegisters.batteryStatus.SLEEP(),
                 balancerRegisters.batteryStatus.DEEPSLEEP(),
                 balancerRegisters.batteryStatus.SA(),
                 balancerRegisters.batteryStatus.SS(),
                 balancerRegisters.batteryStatus.SEC(),
                 balancerRegisters.batteryStatus.FET_EN());
    lampda_print("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                 balancerRegisters.alarmEnable.FULLSCAN(),
                 balancerRegisters.alarmEnable.ADSCAN(),
                 balancerRegisters.alarmEnable.WAKE(),
                 balancerRegisters.alarmEnable.SLEEP(),
                 balancerRegisters.alarmEnable.TIMER_ALARM(),
                 balancerRegisters.alarmEnable.INITCOMP(),
                 balancerRegisters.alarmEnable.CDTOGGLE(),
                 balancerRegisters.alarmEnable.POR(),
                 balancerRegisters.alarmEnable.SSB(),
                 balancerRegisters.alarmEnable.SAA(),
                 balancerRegisters.alarmEnable.SAB(),
                 balancerRegisters.alarmEnable.XCHG(),
                 balancerRegisters.alarmEnable.XDSG(),
                 balancerRegisters.alarmEnable.SHUTV(),
                 balancerRegisters.alarmEnable.CB());
    lampda_print("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                 balancerRegisters.alarmStatus.FULLSCAN(),
                 balancerRegisters.alarmStatus.ADSCAN(),
                 balancerRegisters.alarmStatus.WAKE(),
                 balancerRegisters.alarmStatus.SLEEP(),
                 balancerRegisters.alarmStatus.TIMER_ALARM(),
                 balancerRegisters.alarmStatus.INITCOMP(),
                 balancerRegisters.alarmStatus.CDTOGGLE(),
                 balancerRegisters.alarmStatus.POR(),
                 balancerRegisters.alarmStatus.SSB(),
                 balancerRegisters.alarmStatus.SAA(),
                 balancerRegisters.alarmStatus.SAB(),
                 balancerRegisters.alarmStatus.XCHG(),
                 balancerRegisters.alarmStatus.XDSG(),
                 balancerRegisters.alarmStatus.SHUTV(),
                 balancerRegisters.alarmStatus.CB());
  */
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

  // all cells too far above should be throttled down
  uint8_t set[batteryCount];
  for (uint8_t i = 0; i < batteryCount; i++)
  {
    // set the cell to balance if too far from the mean
    set[i] = status.batteryVoltages_mV[i] >= (batteryVoltageMin + unbalancedMv);
  }
  // start balancing
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

  /*
    // deactivate external temperature sensing and pullup on TS
    balancerRegisters.settingConfiguration_DA.set_disable_ts_reading();
    balancer.readRegEx(balancerRegisters.regoutControl);
    balancerRegisters.regoutControl.set_TS_ON(0);
    balancer.writeRegEx(balancerRegisters.regoutControl);
  */

  // offset to prevent balancing lockup
  balancerRegisters.tsOffset.set(10);

  // set number of cells
  balancerRegisters.configurateVcell.set(batteryCount);

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
    /*
    // set is balancing status
        balancer.readRegEx(balancerRegisters.alarmStatus);

        const uint32_t balancing = balancerRegisters.cbActiveCells.get();
         lampda_print("%d %d %d %d",
                      balancerRegisters.tsMeasurmentVoltage.get(),
                      balancing,
                      balancerRegisters.alarmStatus.CB(),
                      balancer.isFlagRaised);

        _status.isBalancing[0] = (balancing & 0b000010) != 0x00;
        _status.isBalancing[1] = (balancing & 0b000100) != 0x00;
        _status.isBalancing[2] = (balancing & 0b100000) != 0x00;
    */
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
  delay(10);
  balancerRegisters.deepSleep.send();
}

} // namespace balancer