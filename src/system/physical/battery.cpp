#include "battery.h"

#include "src/system/alerts.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace battery {

constexpr uint8_t liionLevelToBatteryPercent(const float liionLevel) {
  return (liionLevel < 40.0)
             ?
             // fast drop for the last half of the battery
             utils::map(liionLevel, 0.0, 40.0, 0.0, 12.0)
             : (liionLevel < 90.0)
                   // most of the battery level is here, slow slope
                   ? utils::map(liionLevel, 40.0, 90.0, 12.0, 95.0)
                   // battery level > 90
                   : utils::map(liionLevel, 90.0, 100.0, 95.0, 100.0);
}

uint16_t read_battery_level() {
  static uint32_t lastReadTime = 0;
  static uint16_t lastBatteryRead = 0;

  const uint32_t time = millis();
  if (lastReadTime == 0 or time - lastReadTime > 500) {
    lastReadTime = time;
    // TODO: replace with a read from the charging component
    lastBatteryRead = analogRead(BAT21);
  }
  return lastBatteryRead;
}

uint8_t get_raw_battery_level(const bool resetRead) {
  static float lastValue = 0;

  // 3v internal ref, ADC resolution
  static constexpr uint16_t minInValue = batteryMinVoltage *
                                         voltageDividerCoeff * ADC_MAX_VALUE /
                                         internalReferenceVoltage;
  static constexpr uint16_t maxInValue = batteryMaxVoltage *
                                         voltageDividerCoeff * ADC_MAX_VALUE /
                                         internalReferenceVoltage;

  // in bounds with some margin
  const uint16_t pinRead = read_battery_level();
  if (pinRead < minInValue * 0.95 or pinRead > maxInValue * 1.05) {
    AlertManager.raise_alert(Alerts::BATTERY_READINGS_INCOHERENT);
    return 0;
  }
  AlertManager.clear_alert(Alerts::BATTERY_READINGS_INCOHERENT);
  const float batteryVoltage = utils::analogToDividerVoltage(pinRead);
  // init or reset
  if (lastValue == 0 or resetRead) {
    lastValue = batteryVoltage;
  }
  // average on 1 second
  static constexpr float filterValue = (float)LOOP_UPDATE_PERIOD / 2000.0;
  lastValue += filterValue * (batteryVoltage - lastValue);

  const float rawBatteryLevel = constrain(
      utils::map(lastValue, batteryMinVoltage, batteryMaxVoltage, 0.0, 100.0),
      0.0, 100.0);
  return liionLevelToBatteryPercent(rawBatteryLevel);
}

uint8_t get_battery_level(const bool resetRead) {
  constexpr uint8_t trueBatteryMaxLevel =
      liionLevelToBatteryPercent(batteryMaxVoltageSafe);
  constexpr uint8_t trueBatteryMinLevel =
      liionLevelToBatteryPercent(batteryMinVoltageSafe);

  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return constrain(utils::map(get_raw_battery_level(resetRead),
                              trueBatteryMinLevel, trueBatteryMaxLevel, 0, 100),
                   0, 100);
}

// Raise the battery low or battery critical alert
void raise_battery_alert() {
  static constexpr uint32_t refreshRate_ms = 1000;
  static uint32_t lastCall = 0;

  const uint32_t newCall = millis();
  if (newCall - lastCall > refreshRate_ms or lastCall == 0) {
    lastCall = newCall;
    const uint8_t percent = get_battery_level();

    // % battery is critical
    if (percent <= batteryCritical) {
      AlertManager.raise_alert(Alerts::BATTERY_CRITICAL);
    } else if (percent > batteryCritical + 1) {
      AlertManager.clear_alert(Alerts::BATTERY_CRITICAL);

      // % battery is low, start alerting
      if (percent <= batteryLow) {
        AlertManager.raise_alert(Alerts::BATTERY_LOW);
      } else if (percent > batteryLow + 1) {
        AlertManager.clear_alert(Alerts::BATTERY_LOW);
      }
    }
  }
}

}  // namespace battery
