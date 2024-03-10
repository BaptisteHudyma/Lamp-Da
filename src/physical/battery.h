#ifndef BATTERY_H
#define BATTERY_H

#include "../alerts.h"
#include "../utils/constants.h"
#include "../utils/utils.h"

static volatile uint8_t batteryLevel = 0.0;

// return a number between 0 and 100
inline uint8_t get_battery_level(const bool resetRead = false) {
  constexpr float maxVoltage = 4.2 * 4;
  constexpr float lowVoltage = 3.1 * 4;

  static float lastValue = 0;

  // map the input ADC out to voltage reading (calibration depending on the
  // resistor used for the battery voltage measurments).
  constexpr float minInValue = 560.0;
  constexpr float maxInValue =
      minInValue / lowVoltage *
      maxVoltage;  // should be linear (voltage divider)
  const uint32_t pinRead = analogRead(BATTERY_CHARGE_PIN);

  if (pinRead < minInValue or pinRead > maxInValue) {
    AlertManager.raise_alert(Alerts::BATTERY_READINGS_INCOHERENT);
  }
  const float batteryVoltage =
      utils::map(pinRead, minInValue, maxInValue, lowVoltage, maxVoltage);

  // init or reset
  if (lastValue == 0 or resetRead) {
    lastValue = batteryVoltage;
  }

  // filter by 1/10
  lastValue += 0.1 * (batteryVoltage - lastValue);
  const float rawBatteryLevel =
      utils::map(lastValue, lowVoltage, maxVoltage, 0.0, 100.0);

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

  return batteryLevel;
}

#endif