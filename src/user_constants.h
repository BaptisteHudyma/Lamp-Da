#ifndef USER_CONSTANTS
#define USER_CONSTANTS

#include <Arduino.h>

const String SOFTWARE_VERSION = "0.1";  // Update when the soft changes version

constexpr float lampBodyRadius_mm = 25;  // external radius of the lamp body

// parameters of the led strip used
constexpr uint16_t LED_COUNT =
    582;  // How many indexable leds are attached to the controler
constexpr float consWattByMeter = 5;     // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;     // voltage (volts)
constexpr uint16_t ledByMeter = 160.0;   // the indexable led by meters
constexpr float ledStripWidth_mm = 5.2;  // width of the led strip

// physical parameters computations
constexpr float ledSize_mm =
    1.0 / ledByMeter * 1000.0;  // size of the individual led
constexpr float lampBodyCircumpherence_mm =
    2.0 * 3.14159265 * lampBodyRadius_mm;
constexpr float ledStripLenght_mm = LED_COUNT * ledSize_mm;

constexpr float stripXCoordinates =
    lampBodyCircumpherence_mm / ledSize_mm + 0.35;
constexpr float stripYCoordinates =
    ledStripLenght_mm / lampBodyCircumpherence_mm;
constexpr float lampBodyHeight_mm = stripYCoordinates * ledStripWidth_mm;

#endif