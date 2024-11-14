#include "battery.h"

#include "src/system/alerts.h"
#include "src/system/charger/charger.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace battery {

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

uint16_t get_battery_voltage_mV() {
  // 3v internal ref, ADC resolution
  static constexpr uint16_t minInValue = (batteryMinVoltage_mV / 1000.0) *
                                         voltageDividerCoeff * ADC_MAX_VALUE /
                                         internalReferenceVoltage;
  static constexpr uint16_t maxInValue = (batteryMaxVoltage_mV / 1000.0) *
                                         voltageDividerCoeff * ADC_MAX_VALUE /
                                         internalReferenceVoltage;

  // in bounds with some margin
  const uint16_t pinRead = read_battery_level();
  if (pinRead < minInValue * 0.95 or pinRead > maxInValue * 1.05) {
    AlertManager.raise_alert(Alerts::BATTERY_READINGS_INCOHERENT);
    return 0;
  }
  AlertManager.clear_alert(Alerts::BATTERY_READINGS_INCOHERENT);
  return utils::analogToDividerVoltage(pinRead) * 1000;
}

uint8_t get_raw_battery_level(const bool resetRead) {
  uint16_t batteryVoltage = 0;
  const auto& chargerStates = charger::get_state();
  if (chargerStates.areMeasuresOk) {
    batteryVoltage = chargerStates.batteryVoltage_mV;
  } else {
    // imprecise and varying in conditions, so prefer the ADC values
    batteryVoltage = get_battery_voltage_mV();
  }

  return utils::get_battery_level_percent(batteryVoltage);
}

uint8_t get_battery_level(const bool resetRead) {
  // get the result of the total battery life, map it to the safe battery level
  // indicated by user
  return utils::get_battery_level(get_raw_battery_level(resetRead));
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
