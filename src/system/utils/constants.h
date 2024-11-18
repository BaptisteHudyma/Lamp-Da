#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

#ifdef ARDUINO
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif

#ifdef USE_TINYUSB  // For Serial when selecting TinyUSB
#include <Adafruit_TinyUSB.h>
#endif

#endif

#ifdef TARGET_LPC1768
#include <Arduino.h>
#endif

#if defined(ARDUINO_ARCH_RP2040)
#include <stdlib.h>

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "rp2040_pio.h"
#endif

#include <stdint.h>

#include "src/user/constants.h"

const String HARDWARE_VERSION = "1.0";
const String BASE_SOFTWARE_VERSION =
    "0.01";  // Update when the soft changes version

constexpr uint8_t ADC_RES_EXP =
    12;  // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
constexpr uint32_t ADC_MAX_VALUE =
    pow(2, ADC_RES_EXP);                         // corresponding max value
constexpr float internalReferenceVoltage = 3.0;  // 3V

// map the input ADC out to voltage reading
constexpr float voltageMeasurmentResistor1_Ohm = 221000;
constexpr float voltageMeasurmentResistor2_Ohm = 47000;
constexpr float voltageDividerCoeff =
    voltageMeasurmentResistor2_Ohm /
    (voltageMeasurmentResistor1_Ohm + voltageMeasurmentResistor2_Ohm);

// number of batteries for this model
constexpr uint8_t batteryCount = 4;

// max voltage of a single li-ion cell
constexpr uint16_t batteryMaxVoltage_mV = 4200 * batteryCount;
// max voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMaxVoltageSafe_mV = 4060 * batteryCount;
// min voltage of a single li-ion cell
constexpr uint16_t batteryMinVoltage_mV = 3000 * batteryCount;
// min voltage of a li-ion cell to maximise lifetime
constexpr uint16_t batteryMinVoltageSafe_mV = 3300 * batteryCount;

// parameters of the lamp body
constexpr float maxPowerConsumption_A =
    2.6;  // Maxpower draw allowed on the system (Amperes)

constexpr float maxSystemTemp_c = 70;       // max proc temperature, in degrees
constexpr float criticalSystemTemp_c = 80;  // max proc temperature, in degrees

// physical parameters computations
constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0;
constexpr float maxStripConsumption_A = totalCons_Watt / inputVoltage_V;

// compute the expected average loop runtime
constexpr uint32_t LOOP_UPDATE_PERIOD = 10;

constexpr float batteryCritical = 300;  // % *100
constexpr float batteryLow = 500;       // % *100

constexpr uint32_t batteryMaxChargeCurrent_mA = 1000;  // mA

// pins

// The button pin (one button pin to GND, the other to this pin)
static constexpr uint32_t BUTTON_PIN = D6;
// Pins for the led on the button
static constexpr uint32_t BUTTON_RED = D8;
static constexpr uint32_t BUTTON_GREEN = D4;
static constexpr uint32_t BUTTON_BLUE = D7;

#endif
