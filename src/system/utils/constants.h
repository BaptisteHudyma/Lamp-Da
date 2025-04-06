#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cmath>
#include <cstdint>

#ifdef USE_TINYUSB // For Serial when selecting TinyUSB
#include <Adafruit_TinyUSB.h>
#endif

#include <stdint.h>

#include "src/user/constants.h"

// current hardware version
#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 0

// define hardware constants
#if HARDWARE_VERSION_MAJOR == 1 && HARDWARE_VERSION_MINOR == 0
#define IS_HARDWARE_1_0
#endif

// expected firmware version, will not compile if missmatch
#define EXPECTED_FIRMWARE_VERSION_MAJOR 1
#define EXPECTED_FIRMWARE_VERSION_MINOR 0

// increment for every  release
#define SOFTWARE_VERSION_MAJOR 1
#define SOFTWARE_VERSION_MINOR 0

constexpr float c_PI = 3.1415926535897;
constexpr float c_TWO_PI = 6.2831853071795;
constexpr float c_HALF_PI = 1.5707963267948;

using byte = uint8_t;

constexpr uint8_t ADC_RES_EXP = 12;                        // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
static const uint32_t ADC_MAX_VALUE = pow(2, ADC_RES_EXP); // corresponding max value
constexpr float internalReferenceVoltage = 3.0;            // 3V

// map the input ADC out to voltage reading
constexpr float voltageMeasurmentResistor1_Ohm = 221000;
constexpr float voltageMeasurmentResistor2_Ohm = 47000;
constexpr float voltageDividerCoeff =
        voltageMeasurmentResistor2_Ohm / (voltageMeasurmentResistor1_Ohm + voltageMeasurmentResistor2_Ohm);

// number of batteries for this model
constexpr uint8_t batteryCount = 3;

// absolute minimum/maximum singular liion battery voltage
constexpr uint16_t minLiionVoltage_mV = 2900;
constexpr uint16_t maxLiionVoltage_mV = 4200;

constexpr uint16_t minSafeLiionVoltage_mV = 3300;
constexpr uint16_t maxSafeLiionVoltage_mV = 4060;

// max voltage of a single li-ion cell
constexpr uint16_t batteryMaxVoltage_mV = maxLiionVoltage_mV * batteryCount;
// max voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMaxVoltageSafe_mV = maxSafeLiionVoltage_mV * batteryCount;
// min voltage of a single li-ion cell
constexpr uint16_t batteryMinVoltage_mV = minLiionVoltage_mV * batteryCount;
// min voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMinVoltageSafe_mV = minSafeLiionVoltage_mV * batteryCount;

constexpr uint16_t minSingularBatteryVoltage_mV = minLiionVoltage_mV * 0.9;
constexpr uint16_t maxSingularBatteryVoltage_mV = maxLiionVoltage_mV * 1.1;

// absolute minimum/maximum battery pack voltage
constexpr uint16_t minBatteryVoltage_mV = minSingularBatteryVoltage_mV * batteryCount;
constexpr uint16_t maxBatteryVoltage_mV = maxSingularBatteryVoltage_mV * batteryCount;

// parameters of the lamp body
constexpr float maxPowerConsumption_A = 2.6; // Maxpower draw allowed on the system (Amperes)

constexpr float maxSystemTemp_c = 70;      // max proc temperature, in degrees
constexpr float criticalSystemTemp_c = 80; // max proc temperature, in degrees

// physical parameters computations
constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0;
constexpr float maxStripConsumption_A = totalCons_Watt / inputVoltage_V;

// compute the expected average loop runtime (in ms)
constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = 1000 / 80.0;

constexpr float batteryCritical = 300; // % *100
constexpr float batteryLow = 500;      // % *100

constexpr uint32_t batteryMaxChargeCurrent_mA = 1000; // mA

using brightness_t = uint16_t;
constexpr brightness_t maxBrightness = 1024;
// min brightness is always zero

#endif
