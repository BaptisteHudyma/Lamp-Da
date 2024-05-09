#ifndef USER_CONSTANTS
#define USER_CONSTANTS

#include <Arduino.h>

const String SOFTWARE_VERSION = "0.1";  // Update when the soft changes version

// parameters of the led strip used
constexpr float consWattByMeter = 12;  // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;   // voltage (volts)
constexpr float ledStripLenght_mm = 91.0 * 25.0;  // 91 sections of 25 mm

#endif