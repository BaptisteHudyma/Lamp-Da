#ifndef USER_CONSTANTS
#define USER_CONSTANTS

/** \file user_constants.h
 *  \brief Constants describing the lamp (width, height, led count, etc)
 **/

#include <Arduino.h>

/// Software version string (update it when software version changes)
const String SOFTWARE_VERSION = "0.1";

//
// physical properties of the lamp
//

/// External radius of the lamp body (in millimeters)
static constexpr float lampBodyRadius_mm = 25;

//
// parameters of the LED strip attached to the controller
//

/// How many indexable LEDs are in the LED strip
static constexpr uint16_t LED_COUNT = 582;
/// Power consumption of the LED strip (in watt/meters)
static constexpr float consWattByMeter = 5;
/// Voltage of the LED strip (in volts)
static constexpr float inputVoltage_V = 12;
/// Number of indexable LEDs by meter of LED strip
static constexpr uint16_t ledByMeter = 160.0;
/// Width of the LED strip
static constexpr float ledStripWidth_mm = 5.2;

//
// physical parameters computations
//

/// Computed circumference of the lamp body (in millimeters)
static constexpr float lampBodyCircumpherence_mm = 2.0 * 3.14159265 * lampBodyRadius_mm;

/// Computed size of an individual LED (in millimeters)
static constexpr float ledSize_mm = 1000.0 / ledByMeter;
/// Computed length of the LED strip (in millimeters)
static constexpr float ledStripLenght_mm = LED_COUNT * ledSize_mm;

/// Computed "X width" (i.e. count of LEDs / columns to wrap around the lamp)
static constexpr float stripXCoordinates = 0.35 + lampBodyCircumpherence_mm / ledSize_mm;
/// Computed "Y height" (i.e. count of LEDs / rows from top to bottom)
static constexpr float stripYCoordinates = ledStripLenght_mm / lampBodyCircumpherence_mm;

/// Computed height of the lamp body (in millimeters)
static constexpr float lampBodyHeight_mm = stripYCoordinates * ledStripWidth_mm;

#endif
