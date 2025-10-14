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
#if HARDWARE_VERSION_MAJOR == 1 && HARDWARE_VERSION_MINOR == 1
#define IS_HARDWARE_1_1
#endif

// expected firmware version, will not compile if missmatch
// Update your dependencies : adafruit lampda_nrf52840
#define EXPECTED_FIRMWARE_VERSION_MAJOR 1
#define EXPECTED_FIRMWARE_VERSION_MINOR 1

// Base software version, common to all systems
// increment for every  release
static constexpr uint8_t BASE_SOFTWARE_VERSION_MAJOR = 1;
static constexpr uint8_t BASE_SOFTWARE_VERSION_MINOR = 5;

using byte = uint8_t;

static constexpr uint8_t ADC_RES_EXP = 12; // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
static const uint32_t ADC_MAX_VALUE = static_cast<uint32_t>(pow(2, ADC_RES_EXP)); // corresponding max value
static constexpr float internalReferenceVoltage = 3.0f;                           // 3V

// number of batteries for this model
static constexpr uint8_t batteryCount = 3;

// define position of imu to the circuit center
static constexpr float imuToCircuitRotationX_rad = 0.0f * c_degreesToRadians;
static constexpr float imuToCircuitRotationY_rad = 0.0f * c_degreesToRadians;
#ifdef IS_HARDWARE_1_0
static constexpr float imuToCircuitRotationZ_rad = -9.0f * c_degreesToRadians;
#else
static constexpr float imuToCircuitRotationZ_rad = -4 * c_degreesToRadians;
#endif
static constexpr float imuToCircuitPositionX_m = 8.915f / 1000.0f;
static constexpr float imuToCircuitPositionY_m = -5.769f / 1000.0f;
static constexpr float imuToCircuitPositionZ_m = 0.0f / 1000.0f;

// battery specific constants
static constexpr float batteryChargeC_Rate = 0.33f;
static constexpr float batteryDischargeC_Rate = 1.0f;
static constexpr uint32_t batteryCapacity_mAH = 4850; // mA hours

static constexpr uint16_t minLiionVoltage_mV = 2500;
static constexpr uint16_t typicalLiionVoltage_mV = 3630;
static constexpr uint16_t maxLiionVoltage_mV = 4200;

// safe single cell voltage to use
static constexpr uint16_t minSafeLiionVoltage_mV = 3100;
static constexpr uint16_t maxSafeLiionVoltage_mV = 4100;

static constexpr uint16_t minSingularBatteryVoltage_mV = static_cast<uint16_t>(minLiionVoltage_mV * 0.9f);
static constexpr uint16_t maxSingularBatteryVoltage_mV = static_cast<uint16_t>(maxLiionVoltage_mV * 1.01f);

// max temp setting
static constexpr float maxSystemTemp_c = 70.0f;      // max proc temperature, in degrees
static constexpr float criticalSystemTemp_c = 80.0f; // max proc temperature, in degrees

// battery settings
static constexpr uint16_t batteryCritical = 300; // % *100
static constexpr uint16_t batteryLow = 500;      // % *100

// watchdog ids
static constexpr uint8_t USER_WATCHDOG_ID = 0;  // user thread watchdog
static constexpr uint8_t POWER_WATCHDOG_ID = 2; // power thread watchdog

/**
 *
 *      Computations from set constants
 *
 */

// EXEPCTED delta time for each loop run
static constexpr float loopDeltaTime = MAIN_LOOP_UPDATE_PERIOD_MS / 1000.0f;

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

// physical parameters computations
static constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0f;
static constexpr float maxStripConsumption_A = 1000.0f * totalCons_Watt / stripInputVoltage_mV;

// compute battery limits from c-rates
static constexpr uint32_t batteryMaxChargeCurrent_mA =
        static_cast<uint32_t>(batteryChargeC_Rate * batteryCapacity_mAH); // mA
static constexpr uint32_t batteryMaxDischargeCurrent_mA =
        static_cast<uint32_t>(batteryDischargeC_Rate * batteryCapacity_mAH); // mA

static constexpr uint32_t batteryTypicalPower_mWH = batteryCapacity_mAH * batteryTypicalVoltageSafe_mV / 1000;

/**
 * \brief check battery cell voltage against min/max limits
 */
inline bool is_cell_voltage_valid(const uint16_t cellVoltage_mv)
{
  return cellVoltage_mv > minSingularBatteryVoltage_mV && cellVoltage_mv < maxSingularBatteryVoltage_mV;
}

using brightness_t = uint16_t;
static constexpr brightness_t maxBrightness = 1024;
// min brightness is always zero

// asserts
static_assert(maxSystemTemp_c < criticalSystemTemp_c, "max system temp must be less than critical temp");

#endif
