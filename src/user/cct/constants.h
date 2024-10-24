#ifndef USER_CONSTANTS
#define USER_CONSTANTS

#ifdef LMBD_LAMP_TYPE__CCT

#include <Arduino.h>

const String SOFTWARE_VERSION = "0.1";  // Update when the soft changes version

// parameters of the led strip used
constexpr float consWattByMeter = 10;  // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;   // voltage (volts)
constexpr float ledStripLenght_mm = 67.0 * 27.0;  // 67 sections of 27 mm

#endif

#endif