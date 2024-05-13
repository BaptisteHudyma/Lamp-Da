#include "battery.h"

#include "../alerts.h"
#include "../utils/constants.h"
#include "../utils/utils.h"

namespace battery {

// return a number between 0 and 100
uint8_t get_battery_level(const bool resetRead) {
  constexpr float maxVoltage = 4.2 * 4;
  constexpr float lowVoltage = 3.1 * 4;

  static float lastValue = 0;
  static uint8_t filteredBattLevel = 0;

  // 3v internal ref, ADC resolution
  constexpr uint16_t minInValue =
      lowVoltage * voltageDividerCoeff * ADC_MAX_VALUE / 3.0;
  constexpr uint16_t maxInValue =
      maxVoltage * voltageDividerCoeff * ADC_MAX_VALUE / 3.0;
  const uint16_t pinRead = analogRead(BAT21);

  // in bounds with some margin
  if (pinRead < minInValue * 0.97 or pinRead > maxInValue * 1.03) {
    AlertManager.raise_alert(Alerts::BATTERY_READINGS_INCOHERENT);
    return 0;
  }
  const float batteryVoltage =
      utils::map(pinRead, minInValue, maxInValue, lowVoltage, maxVoltage);

  // init or reset
  if (lastValue == 0 or resetRead) {
    lastValue = batteryVoltage;
  }

  // average on 1 second
  static constexpr float filterValue = (float)LOOP_UPDATE_PERIOD / 2000.0;
  lastValue += filterValue * (batteryVoltage - lastValue);

  const float rawBatteryLevel = constrain(
      utils::map(lastValue, lowVoltage, maxVoltage, 0.0, 100.0), 0.0, 100.0);

  uint8_t batteryLevel = 0.0;
  // remap to match the reality
  if (rawBatteryLevel < 40.0) {
    // fast drop for the last half of the battery
    batteryLevel = utils::map(rawBatteryLevel, 0.0, 40.0, 0.0, 12.0);
  } else if (rawBatteryLevel < 90.0) {
    // most of the battery level is here, slow slope
    batteryLevel = utils::map(rawBatteryLevel, 40.0, 90.0, 12.0, 95.0);
  } else {
    // battery level > 90
    batteryLevel = utils::map(rawBatteryLevel, 90.0, 105.0, 95.0,
                              100.0);  // highest 15% -> drop slowly
  }

  if (filteredBattLevel == 0) {
    filteredBattLevel = batteryLevel;
  }

  filteredBattLevel +=
      filterValue * filteredBattLevel + (1.0 - filterValue) * batteryLevel;

  return batteryLevel;
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