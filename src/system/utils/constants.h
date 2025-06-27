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

// Base software version, common to all systems
// increment for every  release
#define BASE_SOFTWARE_VERSION_MAJOR 1
#define BASE_SOFTWARE_VERSION_MINOR 2

using byte = uint8_t;

constexpr uint8_t ADC_RES_EXP = 12;                        // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
static const uint32_t ADC_MAX_VALUE = pow(2, ADC_RES_EXP); // corresponding max value
constexpr float internalReferenceVoltage = 3.0;            // 3V

// number of batteries for this model
constexpr uint8_t batteryCount = 3;

// define position of imu to the circuit center
constexpr float imuToCircuitRotationX_rad = 0 * c_degreesToRadians;
constexpr float imuToCircuitRotationY_rad = 0 * c_degreesToRadians;
#ifdef IS_HARDWARE_1_0
constexpr float imuToCircuitRotationZ_rad = -9 * c_degreesToRadians;
#else
constexpr float imuToCircuitRotationZ_rad = -4 * c_degreesToRadians;
#endif
constexpr float imuToCircuitPositionX_m = 8.915 / 1000.0;
constexpr float imuToCircuitPositionY_m = -5.769 / 1000.0;
constexpr float imuToCircuitPositionZ_m = 0 / 1000.0;

// battery specific constants
constexpr float batteryChargeC_Rate = 0.33;
constexpr float batteryDischargeC_Rate = 1.0;
constexpr uint32_t batteryCapacity_mAH = 4850; // mA hours

constexpr uint16_t minLiionVoltage_mV = 2500;
constexpr uint16_t typicalLiionVoltage_mV = 3630;
constexpr uint16_t maxLiionVoltage_mV = 4200;

// safe single cell voltage to use
constexpr uint16_t minSafeLiionVoltage_mV = 3100;
constexpr uint16_t maxSafeLiionVoltage_mV = 4100;

constexpr uint16_t minSingularBatteryVoltage_mV = minLiionVoltage_mV * 0.9;
constexpr uint16_t maxSingularBatteryVoltage_mV = maxLiionVoltage_mV * 1.01;

// max temp setting
constexpr float maxSystemTemp_c = 70;      // max proc temperature, in degrees
constexpr float criticalSystemTemp_c = 80; // max proc temperature, in degrees

// battery settings
constexpr float batteryCritical = 300; // % *100
constexpr float batteryLow = 500;      // % *100

// compute the expected average loop runtime (in ms)
// defined as milliseconds / FPS
constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = 1000 / 80.0;

/**
 *
 *      Computations from set constants
 *
 */

// max voltage of a single li-ion cell
constexpr uint16_t batteryMaxVoltage_mV = maxLiionVoltage_mV * batteryCount;
// max voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMaxVoltageSafe_mV = maxSafeLiionVoltage_mV * batteryCount;
// typical pack voltage
constexpr uint16_t batteryTypicalVoltageSafe_mV = typicalLiionVoltage_mV * batteryCount;
// min voltage of a single li-ion cell
constexpr uint16_t batteryMinVoltage_mV = minLiionVoltage_mV * batteryCount;
// min voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMinVoltageSafe_mV = minSafeLiionVoltage_mV * batteryCount;

// absolute minimum/maximum battery pack voltage
constexpr uint16_t minBatteryVoltage_mV = minSingularBatteryVoltage_mV * batteryCount;
constexpr uint16_t maxBatteryVoltage_mV = maxSingularBatteryVoltage_mV * batteryCount;

// physical parameters computations
constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0;
constexpr float maxStripConsumption_A = totalCons_Watt / inputVoltage_V;

// compute battery limits from c-rates
constexpr uint32_t batteryMaxChargeCurrent_mA = batteryChargeC_Rate * batteryCapacity_mAH;       // mA
constexpr uint32_t batteryMaxDischargeCurrent_mA = batteryDischargeC_Rate * batteryCapacity_mAH; // mA

constexpr uint32_t batteryTypicalPower_mWH = batteryCapacity_mAH * batteryTypicalVoltageSafe_mV / 1000;

using brightness_t = uint16_t;
constexpr brightness_t maxBrightness = 1024;
// min brightness is always zero

#endif
