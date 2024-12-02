#include "battery.h"

#include "src/system/alerts.h"
#include "src/system/charger/charger.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/time.h"

namespace battery {

uint16_t read_battery_mV()
{
  static uint32_t lastReadTime = 0;
  static uint16_t lastBatteryRead = 0;

  const uint32_t time = time_ms();
  if (lastReadTime == 0 or time - lastReadTime > 500)
  {
    lastReadTime = time;
    // read and convert to voltage
    lastBatteryRead = (utils::analogReadToVoltage(analogRead(BAT21)) / voltageDividerCoeff) * 1000;
  }
  return lastBatteryRead;
}

uint16_t get_raw_battery_level()
{
  uint16_t batteryVoltage_mV = 0;
  const auto& chargerStates = charger::get_state();
  if (chargerStates.areMeasuresOk)
  {
    // values from the ADC in the charging component
    batteryVoltage_mV = chargerStates.batteryVoltage_mV;
  }
  else
  {
    // imprecise and varying in conditions, so prefer the ADC values
    batteryVoltage_mV = read_battery_mV();
  }

  // in bounds with some margin
  static constexpr uint16_t minInValue = batteryMinVoltage_mV * 0.95;
  static constexpr uint16_t maxInValue = batteryMaxVoltage_mV * 1.05;
  if (batteryVoltage_mV < minInValue or batteryVoltage_mV > maxInValue)
  {
    AlertManager.raise_alert(Alerts::BATTERY_READINGS_INCOHERENT);
    // return a default low value
    return batteryMinVoltage_mV;
  }
  AlertManager.clear_alert(Alerts::BATTERY_READINGS_INCOHERENT);

  return utils::get_battery_level_percent(batteryVoltage_mV);
}

uint16_t get_battery_level()
{
  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return utils::get_battery_level(get_raw_battery_level());
}

// Raise the battery low or battery critical alert
void raise_battery_alert()
{
  static constexpr uint32_t refreshRate_ms = 1000;
  static uint32_t lastCall = 0;

  const uint32_t newCall = time_ms();
  if (newCall - lastCall > refreshRate_ms or lastCall == 0)
  {
    lastCall = newCall;
    const uint16_t percent = get_battery_level();

    // % battery is critical
    if (percent <= batteryCritical)
    {
      AlertManager.raise_alert(Alerts::BATTERY_CRITICAL);
    }
    else if (percent > batteryCritical + 1)
    {
      AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);

      // % battery is low, start alerting
      if (percent <= batteryLow)
      {
        AlertManager.raise_alert(Alerts::BATTERY_LOW);
      }
      else if (percent > batteryLow + 1)
      {
        AlertManager.clear_alert(Alerts::BATTERY_LOW);
      }
    }
  }
}

} // namespace battery
