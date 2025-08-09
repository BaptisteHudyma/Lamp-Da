#ifndef SYSTEM_UTILS_CONSTANTS_H
#define SYSTEM_UTILS_CONSTANTS_H

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

static constexpr uint8_t ADC_RES_EXP = 12;                 // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
static const uint32_t ADC_MAX_VALUE = pow(2, ADC_RES_EXP); // corresponding max value
static constexpr float internalReferenceVoltage = 3.0;     // 3V

// number of batteries for this model
static constexpr uint8_t batteryCount = 3;

// define position of imu to the circuit center
static constexpr float imuToCircuitRotationX_rad = 0 * c_degreesToRadians;
static constexpr float imuToCircuitRotationY_rad = 0 * c_degreesToRadians;
#ifdef IS_HARDWARE_1_0
static constexpr float imuToCircuitRotationZ_rad = -9 * c_degreesToRadians;
#else
static constexpr float imuToCircuitRotationZ_rad = -4 * c_degreesToRadians;
#endif
static constexpr float imuToCircuitPositionX_m = 8.915 / 1000.0;
static constexpr float imuToCircuitPositionY_m = -5.769 / 1000.0;
static constexpr float imuToCircuitPositionZ_m = 0 / 1000.0;

// battery specific constants
static constexpr float batteryChargeC_Rate = 0.33;
static constexpr float batteryDischargeC_Rate = 1.0;
static constexpr uint32_t batteryCapacity_mAH = 4850; // mA hours

static constexpr uint16_t minLiionVoltage_mV = 2500;
static constexpr uint16_t typicalLiionVoltage_mV = 3630;
static constexpr uint16_t maxLiionVoltage_mV = 4200;

// safe single cell voltage to use
static constexpr uint16_t minSafeLiionVoltage_mV = 3100;
static constexpr uint16_t maxSafeLiionVoltage_mV = 4100;

static constexpr uint16_t minSingularBatteryVoltage_mV = minLiionVoltage_mV * 0.9;
static constexpr uint16_t maxSingularBatteryVoltage_mV = maxLiionVoltage_mV * 1.01;

// max temp setting
static constexpr float maxSystemTemp_c = 70;      // max proc temperature, in degrees
static constexpr float criticalSystemTemp_c = 80; // max proc temperature, in degrees

// battery settings
static constexpr float batteryCritical = 300; // % *100
static constexpr float batteryLow = 500;      // % *100

/**
 *
 *      Computations from set constants
 *
 */

// EXEPCTED delta time for each loop run
static constexpr float loopDeltaTime = MAIN_LOOP_UPDATE_PERIOD_MS / 1000.0;

// max voltage of a single li-ion cell
static constexpr uint16_t batteryMaxVoltage_mV = maxLiionVoltage_mV * batteryCount;
// max voltage of a li-ion cell to maximise lifetime
static constexpr uint16_t batteryMaxVoltageSafe_mV = maxSafeLiionVoltage_mV * batteryCount;
// typical pack voltage
static constexpr uint16_t batteryTypicalVoltageSafe_mV = typicalLiionVoltage_mV * batteryCount;
// min voltage of a single li-ion cell
static constexpr uint16_t batteryMinVoltage_mV = minLiionVoltage_mV * batteryCount;
// min voltage of a li-ion cell to maximise lifetime
static constexpr uint16_t batteryMinVoltageSafe_mV = minSafeLiionVoltage_mV * batteryCount;

// absolute minimum/maximum battery pack voltage
static constexpr uint16_t minBatteryVoltage_mV = minSingularBatteryVoltage_mV * batteryCount;
static constexpr uint16_t maxBatteryVoltage_mV = maxSingularBatteryVoltage_mV * batteryCount;

// physical parameters computations
static constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0;
static constexpr float maxStripConsumption_A = totalCons_Watt / inputVoltage_V;

// compute battery limits from c-rates
static constexpr uint32_t batteryMaxChargeCurrent_mA = batteryChargeC_Rate * batteryCapacity_mAH;       // mA
static constexpr uint32_t batteryMaxDischargeCurrent_mA = batteryDischargeC_Rate * batteryCapacity_mAH; // mA

static constexpr uint32_t batteryTypicalPower_mWH = batteryCapacity_mAH * batteryTypicalVoltageSafe_mV / 1000;

using brightness_t = uint16_t;
static constexpr brightness_t maxBrightness = 1024;
// min brightness is always zero

#endif
