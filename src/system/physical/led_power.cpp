#include "led_power.h"

#include <cmath>
#include <cstdint>

#include "../utils/constants.h"
#include "../utils/utils.h"

namespace ledpower {

bool isVoltageRegulated = false;

float currentCurrent = 0.0;
float targetVoltageSetpoint = 0.0;

void loop() {
  if (isVoltageRegulated) {
    // update the output voltage value
    float readVoltage = 0.0;
    constexpr uint maxRead = 5;
    for (uint i = 0; i < maxRead; i++) {
      readVoltage += utils::analogToDividerVoltage(analogRead(OUTPUT_VOLTAGE));
    }
    readVoltage /= (float)maxRead;

    // R = U/I
    float estimatedImpedance = readVoltage / currentCurrent;
    if (std::isnan(estimatedImpedance) or estimatedImpedance > 1000.0) {
      // temp values, will be replaced by a true one next iteration, start low
      estimatedImpedance = 50.0;
    }

    // I = U/R
    const float targetCurrentValue = targetVoltageSetpoint / estimatedImpedance;
    // this value will oscillate, but be close enough
    write_current(targetCurrentValue);
  }
}

/**
 * Power on the current driver with a specific current value
 */
void write_current(const float current) {
  currentCurrent = constrain(current, 0, maxPowerConsumption_A);

  // map current value to driver value
  const uint8_t mappedDriverValue =
      utils::map(currentCurrent, 0, maxPowerConsumption_A, 0, 255);

  analogWrite(OUT_BRIGHTNESS, mappedDriverValue);
}

/**
 * Power on the current driver with a soecific brightness value
 */
void write_brightness(const uint8_t brightness) {  // map to current value
  isVoltageRegulated = false;

  const float brightnessToCurrent =
      utils::map(brightness, 0, 255, 0, maxStripConsumption_A);
  write_current(brightnessToCurrent);
}

/**
 * Write a target voltage to the controler (regulated with a pid)
 */
void write_voltage(const float voltage) {
  isVoltageRegulated = true;

  targetVoltageSetpoint = voltage;
}

}  // namespace ledpower