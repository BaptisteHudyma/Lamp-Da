#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

#include "../../user_constants.h"

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

constexpr uint8_t ADC_RES_EXP =
    12;  // resolution of the ADC, in bits (can be 8, 10, 12 or 14)
constexpr uint32_t ADC_MAX_VALUE =
    pow(2, ADC_RES_EXP);  // corresponding max value

// map the input ADC out to voltage reading (calibration depending on the
// resistor used for the battery voltage measurments).
// The exact value is a 5.7021 reduction, but it's never exact
constexpr float voltageDividerCoeff = 1.0 / 5.635;

// parameters of the lamp body
constexpr float maxPowerConsumption_A =
    2.6;  // Maxpower draw allowed on the system (Amperes)

constexpr float maxSystemTemp_c = 70;       // max proc temperature, in degrees
constexpr float criticalSystemTemp_c = 80;  // max proc temperature, in degrees

// physical parameters computations
constexpr float totalCons_Watt = consWattByMeter * ledStripLenght_mm / 1000.0;
constexpr float maxStripConsumption_A = totalCons_Watt / inputVoltage_V;

// compute the expected average loop runtime, scaled with the number of led +25%
// for computations
constexpr uint32_t LOOP_UPDATE_PERIOD = 40;

constexpr float batteryCritical = 3;  // %
constexpr float batteryLow = 5;       // %

// pins

// The button pin (one button pin to GND, the other to this pin)
static constexpr uint32_t BUTTON_PIN = D6;
// Pins for the led on the button
static constexpr uint32_t BUTTON_RED = D8;
static constexpr uint32_t BUTTON_GREEN = D4;
static constexpr uint32_t BUTTON_BLUE = D7;

#endif